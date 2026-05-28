#ifndef DATA_PREPROCESSOR_H
#define DATA_PREPROCESSOR_H

#include "esp_err.h"
#include "project_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fixed embedded preprocessing contract.
 * These values are not experiment switches. They mirror the final
 * centered_11 pipeline validated in the XGBoost notebook.
 */
#define DATA_PREPROCESSOR_WINDOW_SIZE 11
#define DATA_PREPROCESSOR_DATE_MAX_LEN 32
#define DATA_PREPROCESSOR_ACT_EMPTY_VALUE (-1.0f)
#define DATA_PREPROCESSOR_ACTION_TARGET_IDP 0U
#define DATA_PREPROCESSOR_FEATURE_COUNT 38

/**
 * @brief Raw packet received by the firmware before preprocessing.
 *
 * This structure contains only the fields available in the original packet.
 * Temporal features, cyclic angle features and window statistics are generated
 * internally by this component.
 */
typedef struct {
    float d_act;
    float w_act;
    float p_act;
    float perc_act;
    float idp;
    float d;
    float w;
    float p;
    float percent;
    float ang;
    char date[DATA_PREPROCESSOR_DATE_MAX_LEN];
} data_preprocessor_raw_packet_t;

/**
 * @brief Preprocessed feature row generated from one packet in the window.
 *
 * The C field names use English identifiers. The CSV header printed by the
 * component keeps the exact dataset column names used by the training pipeline.
 */
typedef struct {
    float d_act;
    float w_act;
    float p_act;
    float perc_act;
    float idp;
    float d;
    float w;
    float p;
    float percent;
    float ang;
    uint64_t date_ts;
    float sin_ang;
    float cos_ang;
    uint8_t ang_is_calculating;

    uint8_t window_lag5_p_changed_centered_11;
    uint8_t window_lag5_w_changed_centered_11;
    uint8_t window_lag5_d_changed_centered_11;
    uint8_t window_lag5_idp_30_count_centered_11;
    uint32_t window_lag5_total_time_delta_sec_centered_11;
    uint8_t window_lag5_has_action_centered_11;

    uint8_t window_pos5_p_changed_centered_11;
    uint8_t window_pos5_w_changed_centered_11;
    uint8_t window_pos5_d_changed_centered_11;
    uint8_t window_pos5_idp_30_count_centered_11;
    uint32_t window_pos5_total_time_delta_sec_centered_11;
    uint8_t window_pos5_has_action_centered_11;

    float window_lag5_delta_percent_centered_11;
    float window_lag5_range_percent_centered_11;
    float window_lag5_max_delta_percent_centered_11;
    float window_lag5_delta_ang_centered_11;
    float window_lag5_range_ang_centered_11;
    float window_lag5_max_delta_ang_centered_11;

    float window_pos5_delta_percent_centered_11;
    float window_pos5_range_percent_centered_11;
    float window_pos5_max_delta_percent_centered_11;
    float window_pos5_delta_ang_centered_11;
    float window_pos5_range_ang_centered_11;
    float window_pos5_max_delta_ang_centered_11;
} data_preprocessor_feature_row_t;

/**
 * @brief Reset the internal ring buffer.
 *
 * Call this function during firmware startup, when changing the pivot/source,
 * or whenever the current temporal history must be discarded.
 */
void data_preprocessor_reset(void);

/**
 * @brief Get the number of packets currently stored in the ring buffer.
 *
 * @return Number of buffered packets, from 0 to DATA_PREPROCESSOR_WINDOW_SIZE.
 */
size_t data_preprocessor_get_packet_count(void);

/**
 * @brief Check whether the 11-packet window is ready.
 *
 * @return true when the component has enough packets to calculate all features.
 */
bool data_preprocessor_is_window_ready(void);

/**
 * @brief Registers a callback for preprocessor-generated IDP events.
 *
 * This follows the same architecture used by scheduling, rush mode and system
 * monitoring: this module creates an IDP packet and the system manager callback
 * routes it to the correct static IDP handler.
 *
 * @param callback Callback to be registered.
 */
void data_preprocessor_register_callback(const app_callback callback);

/**
 * @brief Calculate the preprocessed feature row for the current center packet.
 *
 * The validated model uses a centered 11-packet window. Therefore, when the
 * ring buffer is full, the packet at index 5 is the row that should be sent to
 * the model. This function does not modify the ring buffer and does not run the
 * model; it only exposes the same feature row currently printed for debugging.
 *
 * @param[out] features Destination structure that receives the center row
 * features.
 *
 * @return ESP_OK when the center row is available, ESP_ERR_INVALID_ARG when
 * features is NULL, or ESP_ERR_INVALID_STATE when the 11-packet window is not
 * ready yet.
 */
esp_err_t data_preprocessor_get_center_feature_row(
    data_preprocessor_feature_row_t *features);

/**
 * @brief Fill the model input vector for the current center packet.
 *
 * The generated m2cgen model expects a plain double array, not the named
 * feature structure. This function converts the current center feature row to
 * the exact 38-value order used during XGBoost training and exported in
 * xgboost_feature_order.h.
 *
 * @param[out] feature_vector Destination array with
 * DATA_PREPROCESSOR_FEATURE_COUNT positions.
 *
 * @return ESP_OK when the vector is filled, ESP_ERR_INVALID_ARG when
 * feature_vector is NULL, or ESP_ERR_INVALID_STATE when the 11-packet window is
 * not ready yet.
 */
esp_err_t data_preprocessor_get_center_feature_vector(
    double feature_vector[DATA_PREPROCESSOR_FEATURE_COUNT]);

/**
 * @brief Store an action packet to be applied to the next IDP 00 status row.
 *
 * The control board must call this function after decoding an IDP 01 command.
 * The values are kept in RAM as a pending action. They are not inserted into the
 * ring buffer immediately because the training dataset associates IDP 01 actions
 * with the next status packet generated by the board.
 *
 * @param d_act Decoded D_ACT value from the IDP 01 command.
 * @param w_act Decoded W_ACT value from the IDP 01 command.
 * @param p_act Decoded P_ACT value from the IDP 01 command.
 * @param perc_act Decoded PERC_ACT value from the IDP 01 command.
 *
 * @return ESP_OK when the pending action is stored.
 */
esp_err_t data_preprocessor_set_pending_action(
    uint8_t d_act,
    uint8_t w_act,
    uint8_t p_act,
    uint16_t perc_act);

/**
 * @brief Check whether an IDP 01 action is waiting for the next IDP 00 row.
 *
 * @return true when a pending action is stored.
 */
bool data_preprocessor_has_pending_action(void);

/**
 * @brief Clear the current pending action without adding it to the ring buffer.
 *
 * Use this function only when the firmware intentionally wants to discard the
 * last IDP 01 command before an IDP 00 status packet consumes it.
 */
void data_preprocessor_clear_pending_action(void);

/**
 * @brief Add a decoded status packet to the preprocessing buffer.
 *
 * This is the preferred firmware API for IDP 00 and IDP 30 status rows. If an
 * IDP 01 action is pending and this status packet has IDP 00, the pending action
 * fills D_ACT/W_ACT/P_ACT/PERC_ACT for this row and is then cleared. Other status
 * packets, including IDP 30, are inserted with ACT fields set to -1.
 *
 * @param idp Decoded IDP value from the status packet.
 * @param d Decoded D value from DWP.
 * @param w Decoded W value from DWP.
 * @param p Decoded P value from DWP.
 * @param percent Decoded PERCENT value.
 * @param ang Current ANG value.
 * @param date Date string in "dd/mm/YYYY_HH:MM:SS" format.
 *
 * @return ESP_OK if the packet was accepted.
 */
esp_err_t data_preprocessor_add_status_packet(
    uint8_t idp,
    uint8_t d,
    uint8_t w,
    uint8_t p,
    uint16_t percent,
    uint16_t ang,
    const char *date);

/**
 * @brief Add a raw packet to the preprocessing buffer.
 *
 * When the 11th packet is received, and on every packet after that, the
 * component calculates the features for the latest 11 packets and prints the
 * complete preprocessed window to the terminal.
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
 * @return ESP_OK if the packet was accepted.
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
    const char *date);

/**
 * @brief Add a raw packet using the public packet structure.
 *
 * This is a convenience wrapper for callers that prefer to fill a structure
 * before sending data to the component.
 *
 * @param packet Pointer to the raw packet.
 *
 * @return ESP_OK if the packet was accepted, ESP_ERR_INVALID_ARG if packet is NULL.
 */
esp_err_t data_preprocessor_add_raw_packet(
    const data_preprocessor_raw_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* DATA_PREPROCESSOR_H */
