#include "utf.h"
#include "logitacker_tx_pay_provider_string_to_mouse.h"
#include "logitacker_devices.h"

#define NRF_LOG_MODULE_NAME TX_PAY_PROVIDER_STRING_TO_MOUSE
#include "nrf_log.h"

NRF_LOG_MODULE_REGISTER();

typedef struct {
    char const * source_string;
    logitacker_mouse_map_u8_str_parser_ctx_t str_parser_ctx;
    hid_mouse_report_t * p_current_hid_report_seq;
    uint32_t remaining_reports_in_sequence;
    logitacker_mouse_map_lang_t language_layout;
    logitacker_devices_unifying_device_t * p_device;
    bool use_USB;
} logitacker_tx_payload_provider_string_ctx_t;

bool provider_string_get_next(logitacker_tx_payload_provider_t *self, nrf_esb_payload_t *p_next_payload);
void provider_string_reset(logitacker_tx_payload_provider_t *self);
bool provider_string_inject_get_next(logitacker_tx_payload_provider_string_ctx_t *self, nrf_esb_payload_t *p_next_payload);
void provider_string_inject_reset(logitacker_tx_payload_provider_string_ctx_t *self);

bool provider_string_get_next_hid_report_seq(logitacker_tx_payload_provider_string_ctx_t *self);

const static int SINGLE_HID_REPORT_SIZE = sizeof(hid_mouse_report_t);
static logitacker_tx_payload_provider_t m_local_provider;
static logitacker_tx_payload_provider_string_ctx_t m_local_ctx;

bool provider_string_get_next_hid_report_seq(logitacker_tx_payload_provider_string_ctx_t *self) {
    uint32_t report_seq_size;
    uint32_t res = logitacker_mouse_map_u8_str_to_hid_reports(&self->str_parser_ctx, self->source_string, &self->p_current_hid_report_seq, &report_seq_size, self->language_layout);
    if (res == NRF_SUCCESS) {
        self->remaining_reports_in_sequence = report_seq_size / SINGLE_HID_REPORT_SIZE;
        NRF_LOG_DEBUG("Reports in next sequence %d", self->remaining_reports_in_sequence);
    }

    NRF_LOG_DEBUG("reports for next rune:");
    NRF_LOG_HEXDUMP_DEBUG(self->p_current_hid_report_seq, report_seq_size); //log !raw! resulting report array

    return res == NRF_SUCCESS;
}

bool provider_string_get_next(logitacker_tx_payload_provider_t *self, nrf_esb_payload_t *p_next_payload) {
    return provider_string_inject_get_next((logitacker_tx_payload_provider_string_ctx_t *) (self->p_ctx), p_next_payload);
}

void provider_string_reset(logitacker_tx_payload_provider_t *self) {
    provider_string_inject_reset((logitacker_tx_payload_provider_string_ctx_t *) (self->p_ctx));
}

bool provider_string_inject_get_next(logitacker_tx_payload_provider_string_ctx_t *self, nrf_esb_payload_t *p_next_payload) {
    if (self->remaining_reports_in_sequence == 0) {
        if (!provider_string_get_next_hid_report_seq(self)) {
            return false;
        }
    }

    p_next_payload->length = SINGLE_HID_REPORT_SIZE;
    p_next_payload->data = (uint8_t *) self->p_current_hid_report_seq;
    self->p_current_hid_report_seq++;
    self->remaining_reports_in_sequence--;

    return true;
}

void provider_string_inject_reset(logitacker_tx_payload_provider_string_ctx_t *self) {
    logitacker_mouse_map_u8_str_parser_reset(&self->str_parser_ctx);
    self->p_current_hid_report_seq = NULL;
    self->remaining_reports_in_sequence = 0;
}


logitacker_tx_payload_provider_t * new_payload_provider_string(bool use_USB, logitacker_devices_unifying_device_t * p_device_caps, logitacker_mouse_map_lang_t lang, char const * const str) {
    m_local_ctx.source_string = str;
    m_local_ctx.p_device = p_device_caps;
    m_local_ctx.language_layout = lang;
    m_local_ctx.use_USB = use_USB;

    m_local_provider.p_ctx = &m_local_ctx;
    m_local_provider.get_next = provider_string_get_next;
    m_local_provider.reset = provider_string_reset;

    return &m_local_provider;
}