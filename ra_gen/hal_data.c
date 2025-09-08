/* generated HAL source file - do not edit */
#include "hal_data.h"

gpt_instance_ctrl_t g_timer0_ctrl;
#if 0
const gpt_extended_pwm_cfg_t g_timer0_pwm_extend =
{
    .trough_ipl          = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT0_COUNTER_UNDERFLOW)
    .trough_irq          = VECTOR_NUMBER_GPT0_COUNTER_UNDERFLOW,
#else
    .trough_irq          = FSP_INVALID_VECTOR,
#endif
    .poeg_link           = GPT_POEG_LINK_POEG0,
    .output_disable      = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger         = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up  = 0,
    .dead_time_count_down = 0,
    .adc_a_compare_match = 0,
    .adc_b_compare_match = 0,
    .interrupt_skip_source = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count  = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc    = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t g_timer0_extend =
        { .gtioca =
        { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
          .gtiocb =
          { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
          .start_source = (gpt_source_t) (GPT_SOURCE_NONE), .stop_source = (gpt_source_t) (GPT_SOURCE_NONE), .clear_source =
                  (gpt_source_t) (GPT_SOURCE_NONE),
          .count_up_source = (gpt_source_t) (GPT_SOURCE_NONE), .count_down_source = (gpt_source_t) (GPT_SOURCE_NONE), .capture_a_source =
                  (gpt_source_t) (GPT_SOURCE_NONE),
          .capture_b_source = (gpt_source_t) (GPT_SOURCE_NONE), .capture_a_ipl = (BSP_IRQ_DISABLED), .capture_b_ipl =
                  (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_A)
    .capture_a_irq       = VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_A,
#else
          .capture_a_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_B)
    .capture_b_irq       = VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_B,
#else
          .capture_b_irq = FSP_INVALID_VECTOR,
#endif
          .compare_match_value =
          { /* CMP_A */(uint32_t) 0x0, /* CMP_B */(uint32_t) 0x0 },
          .compare_match_status = (0U << 1U) | 0U, .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE, .capture_filter_gtiocb =
                  GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg                   = &g_timer0_pwm_extend,
#else
          .p_pwm_cfg = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) false,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
          .gtior_setting.gtior = 0U,
#endif
        };

const timer_cfg_t g_timer0_cfg =
{ .mode = TIMER_MODE_PERIODIC,
/* Actual period: 0.01 seconds. Actual duty: 50%. */.period_counts = (uint32_t) 0x124f80,
  .duty_cycle_counts = 0x927c0, .source_div = (timer_source_div_t) 0, .channel = 0, .p_callback = NULL,
  /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend = &g_timer0_extend,
  .cycle_end_ipl = (3),
#if defined(VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW,
#else
  .cycle_end_irq = FSP_INVALID_VECTOR,
#endif
        };
/* Instance structure to use this module. */
const timer_instance_t g_timer0 =
{ .p_ctrl = &g_timer0_ctrl, .p_cfg = &g_timer0_cfg, .p_api = &g_timer_on_gpt };
/* Control structure for storing the driver's internal state. */
i3c_instance_ctrl_t g_i3c0_ctrl;

/* Extended configuration for this instance of I3C. */
const i3c_extended_cfg_t g_i3c0_cfg_extend =
        { .bitrate_settings =
        {
        /* Standard Open Drain; Actual Frequency (Hz): 1000000, Actual High Period (ns): 162.5. */
        .stdbr = ((26U << R_I3C0_STDBR_SBRHO_Pos) | (134U << R_I3C0_STDBR_SBRLO_Pos))
        /* Standard Push-Pull; Actual Frequency (Hz): 3333333.3, Actual High Period (ns): 162.5. */
        | ((26U << R_I3C0_STDBR_SBRHP_Pos) | (22U << R_I3C0_STDBR_SBRLP_Pos)) | (0U << R_I3C0_STDBR_DSBRPO_Pos),
          /* Extended Open Drain; Actual Frequency (Hz): 1000000, Actual High Period (ns): 162.5. */
          .extbr = ((26U << R_I3C0_EXTBR_EBRHO_Pos) | (134U << R_I3C0_EXTBR_EBRLO_Pos))
          /* Extended Push-Pull; Actual Frequency (Hz): 3333333.3, Actual High Period (ns): 162.5.  */
          | ((26U << R_I3C0_EXTBR_EBRHP_Pos) | (22U << R_I3C0_EXTBR_EBRLP_Pos)),

          .clock_stalling =
          { .assigned_address_phase_enable = 0,
            .transition_phase_enable = 0,
            .parity_phase_enable = 0,
            .ack_phase_enable = 0,
            .clock_stalling_time = 0, }, },
          .ibi_control.hot_join_acknowledge = 0, .ibi_control.notify_rejected_hot_join_requests = 0, .ibi_control.notify_rejected_mastership_requests =
                  0,
          .ibi_control.notify_rejected_interrupt_requests = 0, .bus_free_detection_time = 7, .bus_available_detection_time =
                  160,
          .bus_idle_detection_time = 160000, .timeout_detection_enable = false, .slave_command_response_info =
          { .inband_interrupt_enable = false,
            .mastership_request_enable = 0,
            .hotjoin_request_enable = false,
            .activity_state = I3C_ACTIVITY_STATE_ENTAS0,
            .write_length = 65535,
            .read_length = 65535,
            .ibi_payload_length = 0,
            .write_data_rate = I3C_DATA_RATE_SETTING_2MHZ,
            .read_data_rate = I3C_DATA_RATE_SETTING_2MHZ,
            .clock_data_turnaround = I3C_CLOCK_DATA_TURNAROUND_8NS,
            .read_turnaround_time_enable = false,
            .read_turnaround_time = 0,
            .oscillator_frequency = 0,
            .oscillator_inaccuracy = 0,
            .hdr_ddr_support = false,
            .hdr_tsp_support = false,
            .hdr_tsl_support = false, },
          .resp_irq = VECTOR_NUMBER_I3C0_RESPONSE, .rcv_irq = VECTOR_NUMBER_I3C0_RCV_STATUS, .rx_irq =
                  VECTOR_NUMBER_I3C0_RX,
          .tx_irq = VECTOR_NUMBER_I3C0_TX, .ibi_irq = VECTOR_NUMBER_I3C0_IBI, .eei_irq = VECTOR_NUMBER_I3C0_EEI,

          .ipl = (12),
          .eei_ipl = (12), };

/* Configuration for this instance. */
const i3c_cfg_t g_i3c0_cfg =
{ .channel = 0, .device_type = I3C_DEVICE_TYPE_MAIN_MASTER, .p_callback = g_i3c0_callback,
#if defined(NULL)
    .p_context = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend = &g_i3c0_cfg_extend, };

/* Instance structure to use this module. */
const i3c_instance_t g_i3c0 =
{ .p_ctrl = &g_i3c0_ctrl, .p_cfg = &g_i3c0_cfg, .p_api = &g_i3c_on_i3c };
ether_phy_instance_ctrl_t g_ether_phy0_ctrl;

const ether_phy_extended_cfg_t g_ether_phy0_extended_cfg =
{ .p_target_init = NULL, .p_target_link_partner_ability_get = NULL

};

const ether_phy_cfg_t g_ether_phy0_cfg =
{

.channel = 0,
  .phy_lsi_address = 5, .phy_reset_wait_time = 0x00020000, .mii_bit_access_wait_time = 8, .phy_lsi_type =
          ETHER_PHY_LSI_TYPE_KIT_COMPONENT,
  .flow_control = ETHER_PHY_FLOW_CONTROL_DISABLE, .mii_type = ETHER_PHY_MII_TYPE_RMII, .p_context = NULL, .p_extend =
          &g_ether_phy0_extended_cfg,

};
/* Instance structure to use this module. */
const ether_phy_instance_t g_ether_phy0 =
{ .p_ctrl = &g_ether_phy0_ctrl, .p_cfg = &g_ether_phy0_cfg, .p_api = &g_ether_phy_on_ether_phy };
ether_instance_ctrl_t g_ether0_ctrl;

uint8_t g_ether0_mac_address[6] =
{ 0x74, 0x90, 0x50, 0xb0, 0xdd, 0x2a };

__attribute__((__aligned__(16))) ether_instance_descriptor_t g_ether0_tx_descriptors[4] ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(16))) ether_instance_descriptor_t g_ether0_rx_descriptors[4] ETHER_BUFFER_PLACE_IN_SECTION;

__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer0[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer1[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer2[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer3[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer4[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer5[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer6[1536]ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(32)))uint8_t g_ether0_ether_buffer7[1536]ETHER_BUFFER_PLACE_IN_SECTION;

uint8_t *pp_g_ether0_ether_buffers[8] =
{ (uint8_t*) &g_ether0_ether_buffer0[0],
  (uint8_t*) &g_ether0_ether_buffer1[0],
  (uint8_t*) &g_ether0_ether_buffer2[0],
  (uint8_t*) &g_ether0_ether_buffer3[0],
  (uint8_t*) &g_ether0_ether_buffer4[0],
  (uint8_t*) &g_ether0_ether_buffer5[0],
  (uint8_t*) &g_ether0_ether_buffer6[0],
  (uint8_t*) &g_ether0_ether_buffer7[0], };

const ether_extended_cfg_t g_ether0_extended_cfg_t =
{ .p_rx_descriptors = g_ether0_rx_descriptors, .p_tx_descriptors = g_ether0_tx_descriptors, .eesr_event_filter =
          (ETHER_EESR_EVENT_MASK_RFOF | ETHER_EESR_EVENT_MASK_RDE | ETHER_EESR_EVENT_MASK_FR
                  | ETHER_EESR_EVENT_MASK_TFUF | ETHER_EESR_EVENT_MASK_TDE | ETHER_EESR_EVENT_MASK_TC | 0U),
  .ecsr_event_filter = (0U), };

const ether_cfg_t g_ether0_cfg =
{ .channel = 0, .zerocopy = ETHER_ZEROCOPY_DISABLE, .multicast = ETHER_MULTICAST_ENABLE, .promiscuous =
          ETHER_PROMISCUOUS_DISABLE,
  .flow_control = ETHER_FLOW_CONTROL_DISABLE, .padding = ETHER_PADDING_DISABLE, .padding_offset = 1, .broadcast_filter =
          0,
  .p_mac_address = g_ether0_mac_address,

  .num_tx_descriptors = 4,
  .num_rx_descriptors = 4,

  .pp_ether_buffers = pp_g_ether0_ether_buffers,

  .ether_buffer_size = 1536,

#if defined(VECTOR_NUMBER_EDMAC0_EINT)
                .irq                = VECTOR_NUMBER_EDMAC0_EINT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_priority = (12),

  .p_callback = NULL,
  .p_ether_phy_instance = &g_ether_phy0, .p_context = NULL, .p_extend = &g_ether0_extended_cfg_t, };

/* Instance structure to use this module. */
const ether_instance_t g_ether0 =
{ .p_ctrl = &g_ether0_ctrl, .p_cfg = &g_ether0_cfg, .p_api = &g_ether_on_ether };
adc_instance_ctrl_t g_adc0_ctrl;
const adc_extended_cfg_t g_adc0_cfg_extend =
{ .add_average_count = ADC_ADD_OFF,
  .clearing = ADC_CLEAR_AFTER_READ_ON,
  .trigger = ADC_START_SOURCE_DISABLED,
  .trigger_group_b = ADC_START_SOURCE_DISABLED,
  .double_trigger_mode = ADC_DOUBLE_TRIGGER_DISABLED,
  .adc_vref_control = ADC_VREF_CONTROL_VREFH,
  .enable_adbuf = 0,
#if defined(VECTOR_NUMBER_ADC0_WINDOW_A)
    .window_a_irq        = VECTOR_NUMBER_ADC0_WINDOW_A,
#else
  .window_a_irq = FSP_INVALID_VECTOR,
#endif
  .window_a_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_ADC0_WINDOW_B)
    .window_b_irq      = VECTOR_NUMBER_ADC0_WINDOW_B,
#else
  .window_b_irq = FSP_INVALID_VECTOR,
#endif
  .window_b_ipl = (BSP_IRQ_DISABLED), };
const adc_cfg_t g_adc0_cfg =
{ .unit = 0, .mode = ADC_MODE_SINGLE_SCAN, .resolution = ADC_RESOLUTION_12_BIT, .alignment =
          (adc_alignment_t) ADC_ALIGNMENT_RIGHT,
  .trigger = (adc_trigger_t) 0xF, // Not used
  .p_callback = NULL,
  /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend = &g_adc0_cfg_extend,
#if defined(VECTOR_NUMBER_ADC0_SCAN_END)
    .scan_end_irq        = VECTOR_NUMBER_ADC0_SCAN_END,
#else
  .scan_end_irq = FSP_INVALID_VECTOR,
#endif
  .scan_end_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_ADC0_SCAN_END_B)
    .scan_end_b_irq      = VECTOR_NUMBER_ADC0_SCAN_END_B,
#else
  .scan_end_b_irq = FSP_INVALID_VECTOR,
#endif
  .scan_end_b_ipl = (BSP_IRQ_DISABLED), };
#if ((0) | (0))
const adc_window_cfg_t g_adc0_window_cfg =
{
    .compare_mask        =  0,
    .compare_mode_mask   =  0,
    .compare_cfg         = (adc_compare_cfg_t) ((0) | (0) | (0) | (ADC_COMPARE_CFG_EVENT_OUTPUT_OR)),
    .compare_ref_low     = 0,
    .compare_ref_high    = 0,
    .compare_b_channel   = (ADC_WINDOW_B_CHANNEL_0),
    .compare_b_mode      = (ADC_WINDOW_B_MODE_LESS_THAN_OR_OUTSIDE),
    .compare_b_ref_low   = 0,
    .compare_b_ref_high  = 0,
};
#endif
const adc_channel_cfg_t g_adc0_channel_cfg =
{ .scan_mask = ADC_MASK_CHANNEL_0 | ADC_MASK_CHANNEL_4 | ADC_MASK_CHANNEL_7 | 0,
  .scan_mask_group_b = 0,
  .priority_group_a = ADC_GROUP_A_PRIORITY_OFF,
  .add_mask = 0,
  .sample_hold_mask = 0,
  .sample_hold_states = 24,
#if ((0) | (0))
    .p_window_cfg        = (adc_window_cfg_t *) &g_adc0_window_cfg,
#else
  .p_window_cfg = NULL,
#endif
        };
/* Instance structure to use this module. */
const adc_instance_t g_adc0 =
{ .p_ctrl = &g_adc0_ctrl, .p_cfg = &g_adc0_cfg, .p_channel_cfg = &g_adc0_channel_cfg, .p_api = &g_adc_on_adc };
void g_hal_init(void)
{
    g_common_init ();
}
