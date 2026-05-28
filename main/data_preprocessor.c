#include "data_preprocessor.h"

#include "log.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define HALF_WINDOW (DATA_PREPROCESSOR_WINDOW_SIZE / 2)
#define ANG_FULL_CYCLE 360.0f
#define ANG_CALCULATING_VALUE 655.0f
#define IDP_CONTEXT_VALUE 30.0f
#define FLOAT_EPSILON 0.0001f
#define PI_F 3.14159265358979323846f
#define DATA_PREPROCESSOR_TAG "data_preprocessor"

/*
 * Temporary ring buffer debug for firmware validation.
 * Set to 0 when the preprocessing flow is validated.
 */
#define DATA_PREPROCESSOR_DEBUG_LOGS 1

/*
 * This component has no switches to enable or disable preprocessing steps.
 * It directly implements the final pipeline validated in the notebook:
 * DATE_TS, SEN_ANG, COS_ANG, ANG_IS_CALCULATING and centered_11 features.
 */

typedef struct {
    data_preprocessor_raw_packet_t packet;
    uint64_t date_ts;
} buffer_packet_t;

typedef struct {
    uint8_t p_changed;
    uint8_t w_changed;
    uint8_t d_changed;
    uint8_t idp_30_count;
    uint32_t total_time_delta_sec;
    uint8_t has_action;
} context_side_features_t;

typedef struct {
    float delta;
    float range;
    float max_delta;
} temporal_features_t;

typedef struct {
    uint8_t d_act;
    uint8_t w_act;
    uint8_t p_act;
    uint16_t perc_act;
    bool valid;
} pending_action_t;

static buffer_packet_t s_ring[DATA_PREPROCESSOR_WINDOW_SIZE];
static uint8_t s_ring_start;
static uint8_t s_ring_count;
static pending_action_t s_pending_action;

/**
 * @brief Convert invalid numeric values to the default feature value.
 *
 * @param value Input floating-point value.
 *
 * @return The original value, or 0.0f when the input is NaN or infinity.
 */
static float sanitize_float(float value)
{
    if (isnan(value) || isinf(value)) {
        return 0.0f;
    }

    return value;
}

/**
 * @brief Convert invalid action values to the configured empty action marker.
 *
 * @param value Input action value.
 *
 * @return The original value, or DATA_PREPROCESSOR_ACT_EMPTY_VALUE when invalid.
 */
static float sanitize_action_float(float value)
{
    if (isnan(value) || isinf(value)) {
        return DATA_PREPROCESSOR_ACT_EMPTY_VALUE;
    }

    return value;
}

/**
 * @brief Compare two float values using the component epsilon.
 *
 * @param a First value.
 * @param b Second value.
 *
 * @return true when both values are close enough to be treated as equal.
 */
static bool float_equal(float a, float b)
{
    return fabsf(a - b) <= FLOAT_EPSILON;
}

/**
 * @brief Check whether a packet contains any action value.
 *
 * @param packet Raw packet to inspect.
 *
 * @return true when at least one ACT field is not the empty action marker.
 */
static bool packet_has_action(const data_preprocessor_raw_packet_t *packet)
{
    return !float_equal(packet->d_act, DATA_PREPROCESSOR_ACT_EMPTY_VALUE) ||
           !float_equal(packet->w_act, DATA_PREPROCESSOR_ACT_EMPTY_VALUE) ||
           !float_equal(packet->p_act, DATA_PREPROCESSOR_ACT_EMPTY_VALUE) ||
           !float_equal(packet->perc_act, DATA_PREPROCESSOR_ACT_EMPTY_VALUE);
}

/**
 * @brief Fill the ACT fields with the empty action marker.
 *
 * @param packet Raw packet that will receive empty action values.
 */
static void set_empty_action(data_preprocessor_raw_packet_t *packet)
{
    packet->d_act = DATA_PREPROCESSOR_ACT_EMPTY_VALUE;
    packet->w_act = DATA_PREPROCESSOR_ACT_EMPTY_VALUE;
    packet->p_act = DATA_PREPROCESSOR_ACT_EMPTY_VALUE;
    packet->perc_act = DATA_PREPROCESSOR_ACT_EMPTY_VALUE;
}

/**
 * @brief Apply the pending IDP 01 action to one raw packet.
 *
 * @param packet Raw packet that will receive the pending action values.
 */
static void apply_pending_action(data_preprocessor_raw_packet_t *packet)
{
    packet->d_act = (float)s_pending_action.d_act;
    packet->w_act = (float)s_pending_action.w_act;
    packet->p_act = (float)s_pending_action.p_act;
    packet->perc_act = (float)s_pending_action.perc_act;
}

/**
 * @brief Copy a date string into a fixed-size packet buffer.
 *
 * @param dest Destination buffer.
 * @param src Source date string. NULL is accepted and becomes an empty string.
 */
static void copy_date(char dest[DATA_PREPROCESSOR_DATE_MAX_LEN], const char *src)
{
    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    (void)snprintf(dest, DATA_PREPROCESSOR_DATE_MAX_LEN, "%s", src);
}

/**
 * @brief Convert a civil date into days since Unix epoch.
 *
 * @param year Full year.
 * @param month Month in the range 1-12.
 * @param day Day in the range 1-31.
 *
 * @return Number of days since 1970-01-01.
 */
static int64_t days_from_civil(int year, unsigned month, unsigned day)
{
    year -= month <= 2U;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned adjusted_month = month > 2U ? month - 3U : month + 9U;
    const unsigned doy = (153U * adjusted_month + 2U) / 5U + day - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;

    return (int64_t)era * 146097L + (int64_t)doe - 719468L;
}

/**
 * @brief Check whether a year is a leap year.
 *
 * @param year Full year.
 *
 * @return true when the year is leap.
 */
static bool is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/**
 * @brief Get the number of days in a month.
 *
 * @param month Month in the range 1-12.
 * @param year Full year, used for February in leap years.
 *
 * @return Number of days in the requested month.
 */
static uint8_t days_in_month(int month, int year)
{
    static const uint8_t days_by_month[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    return days_by_month[month - 1];
}

/**
 * @brief Validate parsed date and time fields.
 *
 * @param day Day field.
 * @param month Month field.
 * @param year Year field.
 * @param hour Hour field.
 * @param minute Minute field.
 * @param second Second field.
 *
 * @return true when every field is inside an accepted range.
 */
static bool is_valid_datetime_part(int day, int month, int year, int hour, int minute, int second)
{
    if (year < 1970 || month < 1 || month > 12) {
        return false;
    }

    return year >= 1970 &&
           day >= 1 && day <= days_in_month(month, year) &&
           hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59;
}

/**
 * @brief Parse a firmware DATE string into a Unix timestamp.
 *
 * @param date Date string in "dd/mm/YYYY_HH:MM:SS" format.
 *
 * @return Unix timestamp in seconds, or 0 when parsing fails.
 */
static uint64_t parse_date_ts(const char *date)
{
    int day = 0;
    int month = 0;
    int year = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (date == NULL) {
        return 0;
    }

    const int matched = sscanf(date, "%d/%d/%d_%d:%d:%d", &day, &month, &year, &hour, &minute, &second);

    if (matched != 6 || !is_valid_datetime_part(day, month, year, hour, minute, second)) {
        return 0;
    }

    const int64_t days_since_epoch = days_from_civil(year, (unsigned)month, (unsigned)day);

    if (days_since_epoch < 0) {
        return 0;
    }

    return (uint64_t)days_since_epoch * 86400ULL +
           (uint64_t)hour * 3600ULL +
           (uint64_t)minute * 60ULL +
           (uint64_t)second;
}

/**
 * @brief Calculate a non-negative timestamp delta with uint32_t saturation.
 *
 * @param end_ts End timestamp in seconds.
 * @param start_ts Start timestamp in seconds.
 *
 * @return Difference in seconds, clamped to UINT32_MAX if it is too large.
 */
static uint32_t timestamp_delta_sec(uint64_t end_ts, uint64_t start_ts)
{
    if (end_ts <= start_ts) {
        return 0U;
    }

    const uint64_t delta = end_ts - start_ts;

    if (delta > UINT32_MAX) {
        return UINT32_MAX;
    }

    return (uint32_t)delta;
}

/**
 * @brief Normalize a raw packet in-place before storing it in the ring buffer.
 *
 * @param packet Raw packet to normalize.
 */
static void normalize_packet(data_preprocessor_raw_packet_t *packet)
{
    packet->d_act = sanitize_action_float(packet->d_act);
    packet->w_act = sanitize_action_float(packet->w_act);
    packet->p_act = sanitize_action_float(packet->p_act);
    packet->perc_act = sanitize_action_float(packet->perc_act);
    packet->idp = sanitize_float(packet->idp);
    packet->d = sanitize_float(packet->d);
    packet->w = sanitize_float(packet->w);
    packet->p = sanitize_float(packet->p);
    packet->percent = sanitize_float(packet->percent);
    packet->ang = sanitize_float(packet->ang);
}

/**
 * @brief Append a packet to the internal ring buffer.
 *
 * @param packet Buffered packet containing raw values and parsed DATE_TS.
 */
static void append_to_ring(const buffer_packet_t *packet)
{
    if (s_ring_count < DATA_PREPROCESSOR_WINDOW_SIZE) {
        const uint8_t write_index = (s_ring_start + s_ring_count) % DATA_PREPROCESSOR_WINDOW_SIZE;
        s_ring[write_index] = *packet;
        s_ring_count++;
        return;
    }

    s_ring[s_ring_start] = *packet;
    s_ring_start = (s_ring_start + 1U) % DATA_PREPROCESSOR_WINDOW_SIZE;
}

/**
 * @brief Copy the ring buffer into chronological order.
 *
 * @param ordered Destination array with the latest packets in arrival order.
 */
static void copy_ring_ordered(buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE])
{
    for (uint8_t i = 0; i < s_ring_count; i++) {
        const uint8_t ring_index = (s_ring_start + i) % DATA_PREPROCESSOR_WINDOW_SIZE;
        ordered[i] = s_ring[ring_index];
    }
}

#if DATA_PREPROCESSOR_DEBUG_LOGS
/**
 * @brief Print the current ring buffer content in chronological order.
 */
static void print_ring_buffer_debug(void)
{
    buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE];

    copy_ring_ordered(ordered);

    LOG_DATA(DATA_PREPROCESSOR_TAG, "Ring buffer snapshot (%u/%u):",
             (unsigned)s_ring_count,
             (unsigned)DATA_PREPROCESSOR_WINDOW_SIZE);

    for (uint8_t i = 0; i < s_ring_count; i++) {
        const data_preprocessor_raw_packet_t *packet = &ordered[i].packet;

        LOG_DATA(
            DATA_PREPROCESSOR_TAG,
            "[%02u] IDP=%.0f D=%.0f W=%.0f P=%.0f PERCENT=%.0f ANG=%.0f "
            "D_ACT=%.0f W_ACT=%.0f P_ACT=%.0f PERC_ACT=%.0f DATE=%s DATE_TS=%" PRIu64,
            (unsigned)i,
            packet->idp,
            packet->d,
            packet->w,
            packet->p,
            packet->percent,
            packet->ang,
            packet->d_act,
            packet->w_act,
            packet->p_act,
            packet->perc_act,
            packet->date,
            ordered[i].date_ts);
    }
}
#endif

/**
 * @brief Calculate centered window bounds for one row.
 *
 * @param row_idx Row index inside the 11-sample window.
 * @param start_idx Output start index, inclusive.
 * @param end_idx Output end index, exclusive.
 */
static void get_centered_bounds(uint8_t row_idx, uint8_t *start_idx, uint8_t *end_idx)
{
    *start_idx = row_idx > HALF_WINDOW ? row_idx - HALF_WINDOW : 0U;

    const uint8_t raw_end = row_idx + HALF_WINDOW + 1U;
    *end_idx = raw_end > DATA_PREPROCESSOR_WINDOW_SIZE ? DATA_PREPROCESSOR_WINDOW_SIZE : raw_end;
}

/**
 * @brief Calculate context features for one side of the centered window.
 *
 * Context features exclude the target packet itself. The lag side uses packets
 * before the target, while the pos side uses packets after the target.
 *
 * @param ordered Chronological 11-packet window.
 * @param target_idx Target row index inside the window.
 * @param start_idx Context start index, inclusive.
 * @param end_idx Context end index, exclusive.
 * @param is_pos true for the future side of the centered window.
 * @param out Output context feature summary.
 */
static void summarize_context_side(
    const buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE],
    uint8_t target_idx,
    uint8_t start_idx,
    uint8_t end_idx,
    bool is_pos,
    context_side_features_t *out)
{
    const data_preprocessor_raw_packet_t *target = &ordered[target_idx].packet;

    *out = (context_side_features_t){0};

    for (uint8_t i = start_idx; i < end_idx; i++) {
        if (i == target_idx) {
            continue;
        }

        const data_preprocessor_raw_packet_t *ctx = &ordered[i].packet;

        if (!float_equal(ctx->p, target->p)) {
            out->p_changed = 1U;
        }

        if (!float_equal(ctx->w, target->w)) {
            out->w_changed = 1U;
        }

        if (!float_equal(ctx->d, target->d)) {
            out->d_changed = 1U;
        }

        if (float_equal(ctx->idp, IDP_CONTEXT_VALUE)) {
            out->idp_30_count++;
        }

        if (packet_has_action(ctx)) {
            out->has_action = 1U;
        }
    }

    if (start_idx == end_idx) {
        out->total_time_delta_sec = 0;
        return;
    }

    if (is_pos) {
        out->total_time_delta_sec = timestamp_delta_sec(ordered[end_idx - 1U].date_ts, ordered[target_idx].date_ts);
    } else {
        out->total_time_delta_sec = timestamp_delta_sec(ordered[target_idx].date_ts, ordered[start_idx].date_ts);
    }
}

/**
 * @brief Read one temporal signal from a raw packet.
 *
 * @param packet Raw packet.
 * @param use_ang Select ANG when true, otherwise select PERCENT.
 *
 * @return Selected signal value.
 */
static float signal_value(const data_preprocessor_raw_packet_t *packet, bool use_ang)
{
    return use_ang ? packet->ang : packet->percent;
}

/**
 * @brief Calculate delta, range and maximum step for one signal slice.
 *
 * @param ordered Chronological 11-packet window.
 * @param start_idx Signal start index, inclusive.
 * @param end_idx Signal end index, exclusive.
 * @param use_ang Select ANG when true, otherwise select PERCENT.
 *
 * @return Temporal feature summary for the selected signal.
 */
static temporal_features_t summarize_temporal_signal(
    const buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE],
    uint8_t start_idx,
    uint8_t end_idx,
    bool use_ang)
{
    temporal_features_t out = {0};

    if (start_idx >= end_idx) {
        return out;
    }

    const float first = signal_value(&ordered[start_idx].packet, use_ang);
    float last = first;
    float min_value = first;
    float max_value = first;
    float max_delta = 0.0f;
    float previous = first;

    for (uint8_t i = start_idx; i < end_idx; i++) {
        const float current = signal_value(&ordered[i].packet, use_ang);

        if (current < min_value) {
            min_value = current;
        }

        if (current > max_value) {
            max_value = current;
        }

        if (i > start_idx) {
            const float delta = fabsf(current - previous);

            if (delta > max_delta) {
                max_delta = delta;
            }
        }

        previous = current;
        last = current;
    }

    out.delta = last - first;
    out.range = max_value - min_value;
    out.max_delta = max_delta;

    return out;
}

/**
 * @brief Calculate every preprocessed feature for one row in the window.
 *
 * @param ordered Chronological 11-packet window.
 * @param row_idx Row to preprocess.
 * @param out Output feature row.
 */
static void calculate_features_for_row(
    const buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE],
    uint8_t row_idx,
    data_preprocessor_feature_row_t *out)
{
    const data_preprocessor_raw_packet_t *packet = &ordered[row_idx].packet;
    uint8_t start_idx = 0U;
    uint8_t end_idx = 0U;
    context_side_features_t lag_context;
    context_side_features_t pos_context;

    get_centered_bounds(row_idx, &start_idx, &end_idx);

    const uint8_t lag_start = start_idx;
    const uint8_t lag_end = row_idx;
    const uint8_t pos_start = row_idx + 1U;
    const uint8_t pos_end = end_idx;

    summarize_context_side(ordered, row_idx, lag_start, lag_end, false, &lag_context);
    summarize_context_side(ordered, row_idx, pos_start, pos_end, true, &pos_context);

    const temporal_features_t lag_percent = summarize_temporal_signal(ordered, start_idx, row_idx + 1U, false);
    const temporal_features_t lag_ang = summarize_temporal_signal(ordered, start_idx, row_idx + 1U, true);
    const temporal_features_t pos_percent = summarize_temporal_signal(ordered, row_idx, end_idx, false);
    const temporal_features_t pos_ang = summarize_temporal_signal(ordered, row_idx, end_idx, true);

    *out = (data_preprocessor_feature_row_t){
        .d_act = packet->d_act,
        .w_act = packet->w_act,
        .p_act = packet->p_act,
        .perc_act = packet->perc_act,
        .idp = packet->idp,
        .d = packet->d,
        .w = packet->w,
        .p = packet->p,
        .percent = packet->percent,
        .ang = packet->ang,
        .date_ts = ordered[row_idx].date_ts,
        .window_lag5_p_changed_centered_11 = lag_context.p_changed,
        .window_lag5_w_changed_centered_11 = lag_context.w_changed,
        .window_lag5_d_changed_centered_11 = lag_context.d_changed,
        .window_lag5_idp_30_count_centered_11 = lag_context.idp_30_count,
        .window_lag5_total_time_delta_sec_centered_11 = lag_context.total_time_delta_sec,
        .window_lag5_has_action_centered_11 = lag_context.has_action,
        .window_pos5_p_changed_centered_11 = pos_context.p_changed,
        .window_pos5_w_changed_centered_11 = pos_context.w_changed,
        .window_pos5_d_changed_centered_11 = pos_context.d_changed,
        .window_pos5_idp_30_count_centered_11 = pos_context.idp_30_count,
        .window_pos5_total_time_delta_sec_centered_11 = pos_context.total_time_delta_sec,
        .window_pos5_has_action_centered_11 = pos_context.has_action,
        .window_lag5_delta_percent_centered_11 = lag_percent.delta,
        .window_lag5_range_percent_centered_11 = lag_percent.range,
        .window_lag5_max_delta_percent_centered_11 = lag_percent.max_delta,
        .window_lag5_delta_ang_centered_11 = lag_ang.delta,
        .window_lag5_range_ang_centered_11 = lag_ang.range,
        .window_lag5_max_delta_ang_centered_11 = lag_ang.max_delta,
        .window_pos5_delta_percent_centered_11 = pos_percent.delta,
        .window_pos5_range_percent_centered_11 = pos_percent.range,
        .window_pos5_max_delta_percent_centered_11 = pos_percent.max_delta,
        .window_pos5_delta_ang_centered_11 = pos_ang.delta,
        .window_pos5_range_ang_centered_11 = pos_ang.range,
        .window_pos5_max_delta_ang_centered_11 = pos_ang.max_delta,
    };

    if (float_equal(packet->ang, ANG_CALCULATING_VALUE)) {
        out->sin_ang = 0.0f;
        out->cos_ang = 0.0f;
        out->ang_is_calculating = 1U;
        return;
    }

    const float ang_rad = 2.0f * PI_F * packet->ang / ANG_FULL_CYCLE;
    out->sin_ang = sinf(ang_rad);
    out->cos_ang = cosf(ang_rad);
    out->ang_is_calculating = 0U;
}

/**
 * @brief Print the CSV header with the training dataset column names.
 */
static void print_features_header(void)
{
    puts("D_ACT,W_ACT,P_ACT,PERC_ACT,IDP,D,W,P,PERCENT,ANG,DATE_TS,SEN_ANG,COS_ANG,ANG_IS_CALCULATING,"
         "window_lag5_p_changed_centered_11,window_lag5_w_changed_centered_11,window_lag5_d_changed_centered_11,"
         "window_lag5_idp_30_count_centered_11,window_lag5_total_time_delta_sec_centered_11,window_lag5_has_action_centered_11,"
         "window_pos5_p_changed_centered_11,window_pos5_w_changed_centered_11,window_pos5_d_changed_centered_11,"
         "window_pos5_idp_30_count_centered_11,window_pos5_total_time_delta_sec_centered_11,window_pos5_has_action_centered_11,"
         "window_lag5_delta_PERCENT_centered_11,window_lag5_range_PERCENT_centered_11,window_lag5_max_delta_PERCENT_centered_11,"
         "window_lag5_delta_ANG_centered_11,window_lag5_range_ANG_centered_11,window_lag5_max_delta_ANG_centered_11,"
         "window_pos5_delta_PERCENT_centered_11,window_pos5_range_PERCENT_centered_11,window_pos5_max_delta_PERCENT_centered_11,"
         "window_pos5_delta_ANG_centered_11,window_pos5_range_ANG_centered_11,window_pos5_max_delta_ANG_centered_11");
}

/**
 * @brief Print one preprocessed feature row as CSV.
 *
 * @param features Feature row to print.
 */
static void print_features_row(const data_preprocessor_feature_row_t *features)
{
    printf("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%" PRIu64 ",%.6f,%.6f,%u",
           features->d_act,
           features->w_act,
           features->p_act,
           features->perc_act,
           features->idp,
           features->d,
           features->w,
           features->p,
           features->percent,
           features->ang,
           features->date_ts,
           features->sin_ang,
           features->cos_ang,
           (unsigned)features->ang_is_calculating);

    printf(",%u,%u,%u,%u,%" PRIu32 ",%u",
           (unsigned)features->window_lag5_p_changed_centered_11,
           (unsigned)features->window_lag5_w_changed_centered_11,
           (unsigned)features->window_lag5_d_changed_centered_11,
           (unsigned)features->window_lag5_idp_30_count_centered_11,
           features->window_lag5_total_time_delta_sec_centered_11,
           (unsigned)features->window_lag5_has_action_centered_11);

    printf(",%u,%u,%u,%u,%" PRIu32 ",%u",
           (unsigned)features->window_pos5_p_changed_centered_11,
           (unsigned)features->window_pos5_w_changed_centered_11,
           (unsigned)features->window_pos5_d_changed_centered_11,
           (unsigned)features->window_pos5_idp_30_count_centered_11,
           features->window_pos5_total_time_delta_sec_centered_11,
           (unsigned)features->window_pos5_has_action_centered_11);

    printf(",%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
           features->window_lag5_delta_percent_centered_11,
           features->window_lag5_range_percent_centered_11,
           features->window_lag5_max_delta_percent_centered_11,
           features->window_lag5_delta_ang_centered_11,
           features->window_lag5_range_ang_centered_11,
           features->window_lag5_max_delta_ang_centered_11);

    printf(",%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
           features->window_pos5_delta_percent_centered_11,
           features->window_pos5_range_percent_centered_11,
           features->window_pos5_max_delta_percent_centered_11,
           features->window_pos5_delta_ang_centered_11,
           features->window_pos5_range_ang_centered_11,
           features->window_pos5_max_delta_ang_centered_11);
}

/**
 * @brief Calculate and print the current 11-packet preprocessed window.
 */
static void print_preprocessed_window(void)
{
    buffer_packet_t ordered[DATA_PREPROCESSOR_WINDOW_SIZE];

    copy_ring_ordered(ordered);

    print_features_header();

    for (uint8_t i = 0; i < DATA_PREPROCESSOR_WINDOW_SIZE; i++) {
        data_preprocessor_feature_row_t features;

        calculate_features_for_row(ordered, i, &features);
        print_features_row(&features);
    }
}

/**
 * @brief Reset the internal ring buffer.
 */
void data_preprocessor_reset(void)
{
    memset(s_ring, 0, sizeof(s_ring));
    s_ring_start = 0;
    s_ring_count = 0;
    data_preprocessor_clear_pending_action();
}

/**
 * @brief Get the number of packets currently stored in the ring buffer.
 *
 * @return Number of buffered packets.
 */
size_t data_preprocessor_get_packet_count(void)
{
    return s_ring_count;
}

/**
 * @brief Check whether the preprocessing window is ready.
 *
 * @return true when 11 packets are available.
 */
bool data_preprocessor_is_window_ready(void)
{
    return s_ring_count >= DATA_PREPROCESSOR_WINDOW_SIZE;
}

/**
 * @brief Store an IDP 01 action until the next IDP 00 status row arrives.
 *
 * @param d_act Decoded D_ACT value.
 * @param w_act Decoded W_ACT value.
 * @param p_act Decoded P_ACT value.
 * @param perc_act Decoded PERC_ACT value.
 *
 * @return ESP_OK when the action is stored.
 */
esp_err_t data_preprocessor_set_pending_action(
    uint8_t d_act,
    uint8_t w_act,
    uint8_t p_act,
    uint16_t perc_act)
{
    s_pending_action.d_act = d_act;
    s_pending_action.w_act = w_act;
    s_pending_action.p_act = p_act;
    s_pending_action.perc_act = perc_act;
    s_pending_action.valid = true;

    return ESP_OK;
}

/**
 * @brief Check whether an action is waiting for an IDP 00 status row.
 *
 * @return true when a pending action exists.
 */
bool data_preprocessor_has_pending_action(void)
{
    return s_pending_action.valid;
}

/**
 * @brief Clear the current pending action.
 */
void data_preprocessor_clear_pending_action(void)
{
    s_pending_action = (pending_action_t){0};
}

/**
 * @brief Add a decoded status packet and attach pending ACT values only to IDP 00.
 *
 * @param idp Decoded IDP value.
 * @param d Decoded D value.
 * @param w Decoded W value.
 * @param p Decoded P value.
 * @param percent Decoded PERCENT value.
 * @param ang Current ANG value.
 * @param date Date string in "dd/mm/YYYY_HH:MM:SS" format.
 *
 * @return ESP_OK when the packet is accepted.
 */
esp_err_t data_preprocessor_add_status_packet(
    uint8_t idp,
    uint8_t d,
    uint8_t w,
    uint8_t p,
    uint16_t percent,
    uint16_t ang,
    const char *date)
{
    data_preprocessor_raw_packet_t packet = {
        .idp = (float)idp,
        .d = (float)d,
        .w = (float)w,
        .p = (float)p,
        .percent = (float)percent,
        .ang = (float)ang,
    };

    if (s_pending_action.valid && idp == DATA_PREPROCESSOR_ACTION_TARGET_IDP) {
        apply_pending_action(&packet);
        data_preprocessor_clear_pending_action();
    } else {
        set_empty_action(&packet);
    }

    copy_date(packet.date, date);

    return data_preprocessor_add_raw_packet(&packet);
}

/**
 * @brief Add a raw packet using direct field arguments.
 *
 * @param d_act Raw D_ACT value.
 * @param w_act Raw W_ACT value.
 * @param p_act Raw P_ACT value.
 * @param perc_act Raw PERC_ACT value.
 * @param idp Raw IDP value.
 * @param d Raw D value.
 * @param w Raw W value.
 * @param p Raw P value.
 * @param percent Raw PERCENT value.
 * @param ang Raw ANG value.
 * @param date Date string in "dd/mm/YYYY_HH:MM:SS" format.
 *
 * @return ESP_OK when the packet is accepted.
 */
esp_err_t data_preprocessor_add_packet(
    float d_act,
    float w_act,
    float p_act,
    float perc_act,
    float idp,
    float d,
    float w,
    float p,
    float percent,
    float ang,
    const char *date)
{
    data_preprocessor_raw_packet_t packet = {
        .d_act = d_act,
        .w_act = w_act,
        .p_act = p_act,
        .perc_act = perc_act,
        .idp = idp,
        .d = d,
        .w = w,
        .p = p,
        .percent = percent,
        .ang = ang,
    };

    copy_date(packet.date, date);

    return data_preprocessor_add_raw_packet(&packet);
}

/**
 * @brief Add a raw packet using the public packet structure.
 *
 * @param raw_packet Pointer to the packet to append.
 *
 * @return ESP_OK when accepted, ESP_ERR_INVALID_ARG when raw_packet is NULL.
 */
esp_err_t data_preprocessor_add_raw_packet(const data_preprocessor_raw_packet_t *raw_packet)
{
    if (raw_packet == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    buffer_packet_t buffered_packet = {
        .packet = *raw_packet,
        .date_ts = parse_date_ts(raw_packet->date),
    };

    normalize_packet(&buffered_packet.packet);
    append_to_ring(&buffered_packet);

#if DATA_PREPROCESSOR_DEBUG_LOGS
    print_ring_buffer_debug();
#endif

    if (data_preprocessor_is_window_ready()) {
        print_preprocessed_window();
    }

    return ESP_OK;
}
