/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_ble.c
  * @author  MCD Application Team
  * @brief   BLE Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "tl.h"
#include "app_ble.h"

#include "stm32_seq.h"
#include "shci.h"
#include "stm32_lpm.h"
#include "otp.h"

#include "p2p_client_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/**
 * security parameters structure
 */
typedef struct _tSecurityParams
{
  /**
   * IO capability of the device
   */
  uint8_t ioCapability;

  /**
   * Authentication requirement of the device
   * Man In the Middle protection required?
   */
  uint8_t mitm_mode;

  /**
   * bonding mode of the device
   */
  uint8_t bonding_mode;


  /**
   * minimum encryption key size requirement
   */
  uint8_t encryptionKeySizeMin;

  /**
   * maximum encryption key size requirement
   */
  uint8_t encryptionKeySizeMax;

  /**
   * this flag indicates whether the host has to initiate
   * the security, wait for pairing or does not have any security
   * requirements.
   * 0x00 : no security required
   * 0x01 : host should initiate security by sending the slave security
   *        request command
   * 0x02 : host need not send the clave security request but it
   * has to wait for paiirng to complete before doing any other
   * processing
   */
  uint8_t initiateSecurity;
} tSecurityParams;

/**
 * global context
 * contains the variables common to all
 * services
 */
typedef struct _tBLEProfileGlobalContext
{
  /**
   * security requirements of the host
   */
  tSecurityParams bleSecurityParam;

  /**
   * gap service handle
   */
  uint16_t gapServiceHandle;

  /**
   * device name characteristic handle
   */
  uint16_t devNameCharHandle;

  /**
   * appearance characteristic handle
   */
  uint16_t appearanceCharHandle;

  /**
   * connection handle of the current active connection
   * When not in connection, the handle is set to 0xFFFF
   */
  uint16_t connectionHandle;

  /**
   * length of the UUID list to be used while advertising
   */
  uint8_t advtServUUIDlen;

  /**
   * the UUID list to be used while advertising
   */
  uint8_t advtServUUID[100];
} BleGlobalContext_t;

typedef struct
{
  BleGlobalContext_t BleApplicationContext_legacy;
  APP_BLE_ConnStatus_t Device_Connection_Status;
  uint8_t SwitchOffGPIO_timer_Id;
  uint8_t DeviceServerFound;
} BleApplicationContext_t;

#if OOB_DEMO != 0
typedef struct
{
  uint8_t  Identifier;
  uint16_t L2CAP_Length;
  uint16_t Interval_Min;
  uint16_t Interval_Max;
  uint16_t Latency;
  uint16_t Timeout_Multiplier;
} APP_BLE_p2p_Conn_Update_req_t;
#endif

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#define APPBLE_GAP_DEVICE_NAME_LENGTH 7
#define BD_ADDR_SIZE_LOCAL    6
#define BLE_DEFAULT_PIN         (111111)

/* USER CODE BEGIN PD */

/* Eddystone UUID and TLM frame type */
#define EDDYSTONE_UUID              0xFEAA
#define EDDYSTONE_FRAME_TLM         0x20

#if OOB_DEMO != 0
#define LED_ON_TIMEOUT            (0.005*1000*1000/CFG_TS_TICK_VAL) /**< 5ms */
#endif
/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

static const uint8_t a_MBdAddr[BD_ADDR_SIZE_LOCAL] =
{
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0x0000000000FF)),
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0x00000000FF00) >> 8),
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0x000000FF0000) >> 16),
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0x0000FF000000) >> 24),
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0x00FF00000000) >> 32),
  (uint8_t)((CFG_ADV_BD_ADDRESS & 0xFF0000000000) >> 40)
};

static uint8_t a_BdAddrUdn[BD_ADDR_SIZE_LOCAL];
/**
*   Identity root key used to derive IRK and DHK(Legacy)
*/
static const uint8_t a_BLE_CfgIrValue[16] = CFG_BLE_IR;

/**
* Encryption root key used to derive LTK(Legacy) and CSRK
*/
static const uint8_t a_BLE_CfgErValue[16] = CFG_BLE_ER;

tBDAddr SERVER_REMOTE_BDADDR;
uint8_t SERVER_REMOTE_ADDR_TYPE;

P2PC_APP_ConnHandle_Not_evt_t handleNotification;

static BleApplicationContext_t BleApplicationContext;

#if OOB_DEMO != 0
APP_BLE_p2p_Conn_Update_req_t APP_BLE_p2p_Conn_Update_req;
#endif

/* USER CODE BEGIN PV */

static volatile Eddystone_TLM_Data_t g_eddystone_tlm;
static volatile uint8_t g_scan_enabled = 0;
static volatile Eddystone_TLM_Data_t g_eddystone_tlm;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void BLE_UserEvtRx(void * pPayload);
static void BLE_StatusNot(HCI_TL_CmdStatus_t status);
static void Ble_Tl_Init(void);
static void Ble_Hci_Gap_Gatt_Init(void);
static const uint8_t* BleGetBdAddress(void);
static void Scan_Request(void);
static void Switch_OFF_GPIO(void);

/* USER CODE BEGIN PFP */
static void APP_BLE_Parse_Eddystone_TLM(uint8_t *service_data, uint8_t service_data_len);

/* USER CODE END PFP */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void)
{
  SHCI_CmdStatus_t status;
  //tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  /* USER CODE BEGIN APP_BLE_Init_1 */

  /* USER CODE END APP_BLE_Init_1 */
  SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet =
  {
    {{0,0,0}},                          /**< Header unused */
    {0,                                 /** pBleBufferAddress not used */
     0,                                 /** BleBufferSize not used */
     CFG_BLE_NUM_GATT_ATTRIBUTES,
     CFG_BLE_NUM_GATT_SERVICES,
     CFG_BLE_ATT_VALUE_ARRAY_SIZE,
     CFG_BLE_NUM_LINK,
     CFG_BLE_DATA_LENGTH_EXTENSION,
     CFG_BLE_PREPARE_WRITE_LIST_SIZE,
     CFG_BLE_MBLOCK_COUNT,
     CFG_BLE_MAX_ATT_MTU,
     CFG_BLE_PERIPHERAL_SCA,
     CFG_BLE_CENTRAL_SCA,
     CFG_BLE_LS_SOURCE,
     CFG_BLE_MAX_CONN_EVENT_LENGTH,
     CFG_BLE_HSE_STARTUP_TIME,
     CFG_BLE_VITERBI_MODE,
     CFG_BLE_OPTIONS,
     0,
     CFG_BLE_MAX_COC_INITIATOR_NBR,
     CFG_BLE_MIN_TX_POWER,
     CFG_BLE_MAX_TX_POWER,
     CFG_BLE_RX_MODEL_CONFIG,
     CFG_BLE_MAX_ADV_SET_NBR,
     CFG_BLE_MAX_ADV_DATA_LEN,
     CFG_BLE_TX_PATH_COMPENS,
     CFG_BLE_RX_PATH_COMPENS,
     CFG_BLE_CORE_VERSION,
     CFG_BLE_OPTIONS_EXT,
     CFG_BLE_MAX_ADD_EATT_BEARERS
    }
  };

  /**
   * Initialize Ble Transport Layer
   */
  Ble_Tl_Init();

  /**
   * Do not allow standby in the application
   */
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);

  /**
   * Register the hci transport layer to handle BLE User Asynchronous Events
   */
  UTIL_SEQ_RegTask(1<<CFG_TASK_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, hci_user_evt_proc);

  /**
   * Starts the BLE Stack on CPU2
   */
  status = SHCI_C2_BLE_Init(&ble_init_cmd_packet);
  if (status != SHCI_Success)
  {
    APP_DBG_MSG("  Fail   : SHCI_C2_BLE_Init command, result: 0x%02x\n\r", status);
    /* if you are here, maybe CPU2 doesn't contain STM32WB_Copro_Wireless_Binaries, see Release_Notes.html */
    Error_Handler();
  }
  else
  {
    APP_DBG_MSG("  Success: SHCI_C2_BLE_Init command\n\r");
  }

  /**
   * Initialization of HCI & GATT & GAP layer
   */
  Ble_Hci_Gap_Gatt_Init();

  /**
   * Initialization of the BLE Services
   */
  SVCCTL_Init();

  /**
   * From here, all initialization are BLE application specific
   */
  UTIL_SEQ_RegTask(1<<CFG_TASK_START_SCAN_ID, UTIL_SEQ_RFU, Scan_Request);
  //UTIL_SEQ_RegTask(1<<CFG_TASK_CONN_DEV_1_ID, UTIL_SEQ_RFU, Connect_Request);

  /**
   * Initialization of the BLE App Context
   */
  BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;

  /*Radio mask Activity*/
#if (OOB_DEMO != 0)
  ret = aci_hal_set_radio_activity_mask(0x0020);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_set_radio_activity_mask command, result: 0x%x \n\r", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_set_radio_activity_mask command\n\r");
  }
  APP_DBG_MSG("\n");
#endif
  /**
   * Initialize P2P Client Application
   */
  P2PC_APP_Init();

  /* USER CODE BEGIN APP_BLE_Init_3 */

  /* USER CODE END APP_BLE_Init_3 */

#if (OOB_DEMO != 0)
#endif

#if (OOB_DEMO == 0)
  /**
   * Start scanning
   */
#endif
  /* USER CODE BEGIN APP_BLE_Init_2 */

  /* USER CODE END APP_BLE_Init_2 */
  return;
}

SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(void *pckt)
{
  hci_event_pckt *event_pckt;
  evt_le_meta_event *meta_evt;
  hci_le_connection_complete_event_rp0 * connection_complete_event;
  evt_blecore_aci *blecore_evt;
  hci_le_advertising_report_event_rp0 * le_advertising_event;
  event_pckt = (hci_event_pckt*) ((hci_uart_pckt *) pckt)->data;
  hci_disconnection_complete_event_rp0 *cc = (void *) event_pckt->data;
  uint8_t result;
  uint8_t event_type, event_data_size;
  int k = 0;
  uint8_t adtype, adlength;
#if (OOB_DEMO != 0)
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
#endif

  switch (event_pckt->evt)
  {
    /* USER CODE BEGIN evt */

    /* USER CODE END evt */
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
      {
        handleNotification.P2P_Evt_Opcode = PEER_DISCON_HANDLE_EVT;
        blecore_evt = (evt_blecore_aci*) event_pckt->data;
        /* USER CODE BEGIN EVT_VENDOR */

        /* USER CODE END EVT_VENDOR */
        switch (blecore_evt->ecode)
        {
          /* USER CODE BEGIN ecode */

          /* USER CODE END ecode */

          case ACI_GAP_PROC_COMPLETE_VSEVT_CODE:
            {
              /* USER CODE BEGIN EVT_BLUE_GAP_PROCEDURE_COMPLETE */
            	if (g_scan_enabled)
            	{
            	    APP_DBG_MSG("Scan complete — restarting continuous scan...\r\n");
            	    UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
            	}
            	else
            	{
            	    APP_DBG_MSG("Scan complete — scan disabled.\r\n");
            	}
              /* USER CODE END EVT_BLUE_GAP_PROCEDURE_COMPLETE */
              aci_gap_proc_complete_event_rp0 *gap_evt_proc_complete = (void*) blecore_evt->data;
              /* CHECK GAP GENERAL DISCOVERY PROCEDURE COMPLETED & SUCCEED */
//              if (gap_evt_proc_complete->Procedure_Code == GAP_GENERAL_DISCOVERY_PROC
//                  && gap_evt_proc_complete->Status == 0x00)
//              {
//                /* USER CODE BEGIN GAP_GENERAL_DISCOVERY_PROC */
//                BSP_LED_Off(LED_BLUE);
//                APP_DBG_MSG("LED BLUE OFF\n");
//
//                /* Restart scanning when no P2P server connection is pending. */
//                if ((BleApplicationContext.DeviceServerFound != 0x01) &&
//                    (BleApplicationContext.Device_Connection_Status != APP_BLE_CONNECTED_CLIENT))
//                {
//                  APP_DBG_MSG("Restarting BLE scan for Eddystone-TLM...\r\n");
//                  UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
//                }
//                /* USER CODE END GAP_GENERAL_DISCOVERY_PROC */
//                APP_DBG_MSG("-- GAP GENERAL DISCOVERY PROCEDURE_COMPLETED\n\r");
//                /*if a device found, connect to it, device 1 being chosen first if both found*/
//                if (BleApplicationContext.DeviceServerFound == 0x01 && BleApplicationContext.Device_Connection_Status != APP_BLE_CONNECTED_CLIENT)
//                {
//                  UTIL_SEQ_SetTask(1 << CFG_TASK_CONN_DEV_1_ID, CFG_SCH_PRIO_0);
//                }
//              }
            }
            break;

   #if (OOB_DEMO != 0)
          case ACI_L2CAP_CONNECTION_UPDATE_REQ_VSEVT_CODE:
            {
              /* USER CODE BEGIN EVT_BLUE_L2CAP_CONNECTION_UPDATE_REQ */

              /* USER CODE END EVT_BLUE_L2CAP_CONNECTION_UPDATE_REQ */
              aci_l2cap_connection_update_req_event_rp0 *pr = (aci_l2cap_connection_update_req_event_rp0 *) blecore_evt->data;
              ret = aci_hal_set_radio_activity_mask(0x0000);
              if (ret != BLE_STATUS_SUCCESS)
              {
                APP_DBG_MSG("  Fail   : aci_hal_set_radio_activity_mask command, result: 0x%x \n\r", ret);
              }
              else
              {
                APP_DBG_MSG("  Success: aci_hal_set_radio_activity_mask command\n\r");
              }

              APP_BLE_p2p_Conn_Update_req.Identifier = pr->Identifier;
              APP_BLE_p2p_Conn_Update_req.L2CAP_Length = pr->L2CAP_Length;
              APP_BLE_p2p_Conn_Update_req.Interval_Min = pr->Interval_Min;
              APP_BLE_p2p_Conn_Update_req.Interval_Max = pr->Interval_Max;
              APP_BLE_p2p_Conn_Update_req.Latency = pr->Latency;
              APP_BLE_p2p_Conn_Update_req.Timeout_Multiplier = pr->Timeout_Multiplier;

              ret = aci_l2cap_connection_parameter_update_resp(BleApplicationContext.BleApplicationContext_legacy.connectionHandle,
                                                               APP_BLE_p2p_Conn_Update_req.Interval_Min,
                                                               APP_BLE_p2p_Conn_Update_req.Interval_Max,
                                                               APP_BLE_p2p_Conn_Update_req.Latency,
                                                               APP_BLE_p2p_Conn_Update_req.Timeout_Multiplier,
                                                               CONN_L1,
                                                               CONN_L2,
                                                               APP_BLE_p2p_Conn_Update_req.Identifier,
                                                               0x01);
              if(ret != BLE_STATUS_SUCCESS)
              {
                APP_DBG_MSG("  Fail   : aci_l2cap_connection_parameter_update_resp command, result: 0x%x \n\r", ret);
                /* USER CODE BEGIN BLE_STATUS_SUCCESS */
                BSP_LED_On(LED_RED);
                /* USER CODE END BLE_STATUS_SUCCESS */
              }
              else
              {
                APP_DBG_MSG("  Success: aci_l2cap_connection_parameter_update_resp command\n\r");
              }

              ret = aci_hal_set_radio_activity_mask(0x0020);
              if (ret != BLE_STATUS_SUCCESS)
              {
                APP_DBG_MSG("  Fail   : aci_hal_set_radio_activity_mask command, result: 0x%x \n\r", ret);
              }
              else
              {
                APP_DBG_MSG("  Success: aci_hal_set_radio_activity_mask command\n\r");
              }
            }
            break;

          case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:
            {
              /* USER CODE BEGIN RADIO_ACTIVITY_EVENT */
              BSP_LED_On(LED_GREEN);
              HW_TS_Start(BleApplicationContext.SwitchOffGPIO_timer_Id, (uint32_t)LED_ON_TIMEOUT);
              /* USER CODE END RADIO_ACTIVITY_EVENT */
            }
            break; /* ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE */
  #endif

          /* USER CODE BEGIN BLUE_EVT */

          /* USER CODE END BLUE_EVT */

          default:
            /* USER CODE BEGIN ecode_default */

            /* USER CODE END ecode_default */
            break;
        }
      }
      break;

    case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
      {
        /* USER CODE BEGIN EVT_DISCONN_COMPLETE */

        /* USER CODE END EVT_DISCONN_COMPLETE */
        if (cc->Connection_Handle == BleApplicationContext.BleApplicationContext_legacy.connectionHandle)
        {
          BleApplicationContext.BleApplicationContext_legacy.connectionHandle = 0;
          BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;
          APP_DBG_MSG("\r\n\r** DISCONNECTION EVENT WITH SERVER \n\r");
          handleNotification.P2P_Evt_Opcode = PEER_DISCON_HANDLE_EVT;
          handleNotification.ConnectionHandle = BleApplicationContext.BleApplicationContext_legacy.connectionHandle;
          P2PC_APP_Notification(&handleNotification);
        }
      }
      break; /* HCI_DISCONNECTION_COMPLETE_EVT_CODE */

    case HCI_LE_META_EVT_CODE:
      {
        /* USER CODE BEGIN EVT_LE_META_EVENT */

        /* USER CODE END EVT_LE_META_EVENT */
        meta_evt = (evt_le_meta_event*) event_pckt->data;

        switch (meta_evt->subevent)
        {
          /* USER CODE BEGIN subevent */

          /* USER CODE END subevent */

          case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
            /* USER CODE BEGIN EVT_LE_CONN_COMPLETE */

            /* USER CODE END EVT_LE_CONN_COMPLETE */
            /**
             * The connection is done,
             */
            break; /* HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE */

          case HCI_LE_ADVERTISING_REPORT_SUBEVT_CODE:
            {
              uint8_t *adv_report_data;
              /* USER CODE BEGIN EVT_LE_ADVERTISING_REPORT */

              /* USER CODE END EVT_LE_ADVERTISING_REPORT */
              le_advertising_event = (hci_le_advertising_report_event_rp0 *) meta_evt->data;

              event_type = le_advertising_event->Advertising_Report[0].Event_Type;

              event_data_size = le_advertising_event->Advertising_Report[0].Length_Data;

              /* WARNING: be careful when decoding advertising report as its raw format cannot be mapped on a C structure.
              The data and RSSI values could not be directly decoded from the RAM using the data and RSSI field from hci_le_advertising_report_event_rp0 structure.
              Instead they must be read by using offsets (please refer to BLE specification).
              RSSI = (int8_t)*(uint8_t*) (adv_report_data + le_advertising_event->Advertising_Report[0].Length_Data);
              */
              adv_report_data = (uint8_t*)(&le_advertising_event->Advertising_Report[0].Length_Data) + 1;
              k = 0;

              /* search AD TYPE 0x09 (Complete Local Name) */
              /* search AD Type 0x02 (16 bits UUIDS) */
              /* Process AD structures from all received advertising reports. */
              {
                /* ISOLATION OF BD ADDRESS AND LOCAL NAME */

                while(k < event_data_size)
                {
                  adlength = adv_report_data[k];
                  adtype = adv_report_data[k + 1];
                  switch (adtype)
                  {
                    case AD_TYPE_FLAGS: /* now get flags */
                      /* USER CODE BEGIN AD_TYPE_FLAGS */

                      /* USER CODE END AD_TYPE_FLAGS */
                      break;

                    case AD_TYPE_TX_POWER_LEVEL: /* Tx power level */
                      /* USER CODE BEGIN AD_TYPE_TX_POWER_LEVEL */

                      /* USER CODE END AD_TYPE_TX_POWER_LEVEL */
                      break;

                    case AD_TYPE_MANUFACTURER_SPECIFIC_DATA: /* Manufacturer Specific */
                      /* USER CODE BEGIN AD_TYPE_MANUFACTURER_SPECIFIC_DATA */

                      /* USER CODE END AD_TYPE_MANUFACTURER_SPECIFIC_DATA */
                      if (adlength >= 7 && adv_report_data[k + 2] == 0x01)
                      { /* ST VERSION ID 01 */
                        APP_DBG_MSG("--- ST MANUFACTURER ID --- \n\r");
                        switch (adv_report_data[k + 3])
                        {   /* Demo ID */
                           case CFG_DEV_ID_P2P_SERVER1: /* End Device 1 */
                           APP_DBG_MSG("-- SERVER DETECTED -- VIA MAN ID\n\r");
                           BleApplicationContext.DeviceServerFound = 0x01;
                           SERVER_REMOTE_ADDR_TYPE = le_advertising_event->Advertising_Report[0].Address_Type;
                           SERVER_REMOTE_BDADDR[0] = le_advertising_event->Advertising_Report[0].Address[0];
                           SERVER_REMOTE_BDADDR[1] = le_advertising_event->Advertising_Report[0].Address[1];
                           SERVER_REMOTE_BDADDR[2] = le_advertising_event->Advertising_Report[0].Address[2];
                           SERVER_REMOTE_BDADDR[3] = le_advertising_event->Advertising_Report[0].Address[3];
                           SERVER_REMOTE_BDADDR[4] = le_advertising_event->Advertising_Report[0].Address[4];
                           SERVER_REMOTE_BDADDR[5] = le_advertising_event->Advertising_Report[0].Address[5];
                           break;

                          default:
                            break;
                        }
                      }
                      break;

                    case AD_TYPE_SERVICE_DATA: /* service data 16 bits */
                      /* USER CODE BEGIN AD_TYPE_SERVICE_DATA */

                      /*
                       * AD structure:
                       *   k+0 : AD length
                       *   k+1 : AD type (0x16 = Service Data)
                       *   k+2 : Service UUID LSB
                       *   k+3 : Service UUID MSB
                       *   k+4 : Eddystone frame type
                       *
                       * Pass the bytes after the AD type to the parser.
                       */
                      APP_BLE_Parse_Eddystone_TLM(&adv_report_data[k + 2],
                                                  (uint8_t)(adlength - 1));

                      /* USER CODE END AD_TYPE_SERVICE_DATA */
                      break;

                    default:
                      /* USER CODE BEGIN adtype_default */

                      /* USER CODE END adtype_default */
                      break;
                  } /* end switch adtype */
                  k += adlength + 1;
                } /* end while */
              } /* end if ADV_IND */

              /* USER CODE BEGIN EVT_LE_ADVERTISING_REPORT_2 */

              /* USER CODE END EVT_LE_ADVERTISING_REPORT_2 */
            }
            break;

          /* USER CODE BEGIN META_EVT */

          /* USER CODE END META_EVT */

          default:
            /* USER CODE BEGIN subevent_default */

            /* USER CODE END subevent_default */
            break;
        }
      }
      break; /* HCI_LE_META_EVT_CODE */

    /* USER CODE BEGIN EVENT_PCKT */

    /* USER CODE END EVENT_PCKT */

    default:
      /* USER CODE BEGIN evt_default */

      /* USER CODE END evt_default */
      break;
  }

  return (SVCCTL_UserEvtFlowEnable);
}

APP_BLE_ConnStatus_t APP_BLE_Get_Client_Connection_Status(uint16_t Connection_Handle)
{
  if (BleApplicationContext.BleApplicationContext_legacy.connectionHandle == Connection_Handle)
  {
    return BleApplicationContext.Device_Connection_Status;
  }
  return APP_BLE_IDLE;
}
/* USER CODE BEGIN FD */
void APP_BLE_Key_Button1_Action(void)
{
  g_scan_enabled ^= 1;   // toggle

  if (g_scan_enabled)
  {
    APP_DBG_MSG("Button1: ENABLE scanning\r\n");
    UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
    BSP_LED_On(LED_BLUE);
  }
  else
  {
    APP_DBG_MSG("Button1: DISABLE scanning\r\n");
    aci_gap_terminate_gap_proc(GAP_GENERAL_DISCOVERY_PROC);
    BSP_LED_Off(LED_BLUE);
  }
}

void APP_BLE_Key_Button2_Action(void)
{
}

void APP_BLE_Key_Button3_Action(void)
{
}
/* USER CODE END FD */
/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
static void Ble_Tl_Init(void)
{
  HCI_TL_HciInitConf_t Hci_Tl_Init_Conf;

  Hci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*)&BleCmdBuffer;
  Hci_Tl_Init_Conf.StatusNotCallBack = BLE_StatusNot;
  hci_init(BLE_UserEvtRx, (void*) &Hci_Tl_Init_Conf);

  return;
}

static void Ble_Hci_Gap_Gatt_Init(void)
{
  uint8_t role;
  uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
  const uint8_t *p_bd_addr;

  uint16_t a_appearance[1] = { BLE_CFG_GAP_APPEARANCE };
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;

  APP_DBG_MSG("==>> Start Ble_Hci_Gap_Gatt_Init function\n");

  /**
   * Initialize HCI layer
   */
  /*HCI Reset to synchronise BLE Stack*/
  ret = hci_reset();
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : hci_reset command, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: hci_reset command\n");
  }

  /**
   * Write the BD Address
   */
  p_bd_addr = BleGetBdAddress();
  ret = aci_hal_write_config_data(CONFIG_DATA_PUBLIC_ADDRESS_OFFSET, CONFIG_DATA_PUBLIC_ADDRESS_LEN, (uint8_t*) p_bd_addr);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_PUBLIC_ADDRESS_OFFSET, result: 0x%x \n", ret);
  }
   else
   {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_PUBLIC_ADDRESS_OFFSET\n");
    APP_DBG_MSG("  Public Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",p_bd_addr[5],p_bd_addr[4],p_bd_addr[3],p_bd_addr[2],p_bd_addr[1],p_bd_addr[0]);
   }

  /**
   * Write Identity root key used to derive IRK and DHK(Legacy)
   */
  ret = aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t*)a_BLE_CfgIrValue);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_IR_OFFSET, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_IR_OFFSET\n");
  }

  /**
   * Write Encryption root key used to derive LTK and CSRK
   */
  ret = aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t*)a_BLE_CfgErValue);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_ER_OFFSET, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_ER_OFFSET\n");
  }

  /**
   * Set TX Power.
   */
  ret = aci_hal_set_tx_power_level(1, CFG_TX_POWER);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_set_tx_power_level command, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_set_tx_power_level command\n");
  }

  /**
   * Initialize GATT interface
   */
  ret = aci_gatt_init();
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_init command, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_init command\n");
  }

  /**
   * Initialize GAP interface
   */
  role = 0;

#if (BLE_CFG_PERIPHERAL == 1)
  role |= GAP_PERIPHERAL_ROLE;
#endif /* BLE_CFG_PERIPHERAL == 1 */

#if (BLE_CFG_CENTRAL == 1)
  role |= GAP_CENTRAL_ROLE;
#endif /* BLE_CFG_CENTRAL == 1 */

/* USER CODE BEGIN Role_Mngt*/

/* USER CODE END Role_Mngt */

  if (role > 0)
  {
    const char *name = "P2P_C";

    ret = aci_gap_init(role,
                       CFG_PRIVACY,
                       APPBLE_GAP_DEVICE_NAME_LENGTH,
                       &gap_service_handle,
                       &gap_dev_name_char_handle,
                       &gap_appearance_char_handle);

    if (ret != BLE_STATUS_SUCCESS)
    {
      APP_DBG_MSG("  Fail   : aci_gap_init command, result: 0x%x \n", ret);
    }
    else
    {
      APP_DBG_MSG("  Success: aci_gap_init command\n");
    }

    ret = aci_gatt_update_char_value(gap_service_handle, gap_dev_name_char_handle, 0, strlen(name), (uint8_t *) name);
    if (ret != BLE_STATUS_SUCCESS)
    {
      BLE_DBG_SVCCTL_MSG("  Fail   : aci_gatt_update_char_value - Device Name\n");
    }
    else
    {
      BLE_DBG_SVCCTL_MSG("  Success: aci_gatt_update_char_value - Device Name\n");
    }
  }

  ret = aci_gatt_update_char_value(gap_service_handle,
                                   gap_appearance_char_handle,
                                   0,
                                   2,
                                   (uint8_t *)&a_appearance);
  if (ret != BLE_STATUS_SUCCESS)
  {
    BLE_DBG_SVCCTL_MSG("  Fail   : aci_gatt_update_char_value - Appearance\n");
  }
  else
  {
    BLE_DBG_SVCCTL_MSG("  Success: aci_gatt_update_char_value - Appearance\n");
  }

  /**
   * Initialize IO capability
   */
  BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability = CFG_IO_CAPABILITY;
  ret = aci_gap_set_io_capability(BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_io_capability command, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_io_capability command\n");
  }

  /**
   * Initialize authentication
   */
  BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode = CFG_MITM_PROTECTION;
  BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin = CFG_ENCRYPTION_KEY_SIZE_MIN;
  BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax = CFG_ENCRYPTION_KEY_SIZE_MAX;
  BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode = CFG_BONDING_MODE;

  ret = aci_gap_set_authentication_requirement(BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode,
                                               BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode,
                                               CFG_SC_SUPPORT,
                                               CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                               BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin,
                                               BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax,
                                               USE_FIXED_PIN_FOR_PAIRING_FORBIDDEN, /* deprecated feature */
                                               0,                                   /* deprecated feature */
                                               CFG_IDENTITY_ADDRESS);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_authentication_requirement command, result: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_authentication_requirement command\n");
  }

  /**
   * Initialize whitelist
   */
  if (BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode)
  {
    ret = aci_gap_configure_whitelist();
    if (ret != BLE_STATUS_SUCCESS)
    {
      APP_DBG_MSG("  Fail   : aci_gap_configure_whitelist command, result: 0x%x \n", ret);
    }
    else
    {
      APP_DBG_MSG("  Success: aci_gap_configure_whitelist command\n");
    }
  }
  APP_DBG_MSG("==>> End Ble_Hci_Gap_Gatt_Init function\n\r");
}

static void Scan_Request(void)
{
  /* USER CODE BEGIN Scan_Request_1 */

  if (!g_scan_enabled)
  {
    APP_DBG_MSG("Scan_Request ignored (scan disabled)\r\n");
    return;
  }

  APP_DBG_MSG("Starting BLE scan for Eddystone-TLM...\r\n");

  /* USER CODE END Scan_Request_1 */
  tBleStatus result = aci_gap_start_general_discovery_proc(
      SCAN_P, SCAN_L, CFG_BLE_ADDRESS_TYPE, 1);

  if (result == BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("BLE scanning started.\r\n");
  }
  else
  {
    APP_DBG_MSG("BLE scan start FAILED.\r\n");
  }

  /* USER CODE BEGIN Scan_Request_2 */
  /* USER CODE END Scan_Request_2 */
}


static void Switch_OFF_GPIO()
{
  /* USER CODE BEGIN Switch_OFF_GPIO */
  BSP_LED_Off(LED_GREEN);
  /* USER CODE END Switch_OFF_GPIO */
}

const uint8_t* BleGetBdAddress(void)
{
  uint8_t *p_otp_addr;
  const uint8_t *p_bd_addr;
  uint32_t udn;
  uint32_t company_id;
  uint32_t device_id;

  udn = LL_FLASH_GetUDN();

  if (udn != 0xFFFFFFFF)
  {
    company_id = LL_FLASH_GetSTCompanyID();
    device_id = LL_FLASH_GetDeviceID();

    /**
     * Public Address with the ST company ID
     * bit[47:24] : 24bits (OUI) equal to the company ID
     * bit[23:16] : Device ID.
     * bit[15:0] : The last 16bits from the UDN
     * Note: In order to use the Public Address in a final product, a dedicated
     * 24bits company ID (OUI) shall be bought.
     */
    a_BdAddrUdn[0] = (uint8_t)(udn & 0x000000FF);
    a_BdAddrUdn[1] = (uint8_t)((udn & 0x0000FF00) >> 8);
    a_BdAddrUdn[2] = (uint8_t)device_id;
    a_BdAddrUdn[3] = (uint8_t)(company_id & 0x000000FF);
    a_BdAddrUdn[4] = (uint8_t)((company_id & 0x0000FF00) >> 8);
    a_BdAddrUdn[5] = (uint8_t)((company_id & 0x00FF0000) >> 16);

    p_bd_addr = (const uint8_t *)a_BdAddrUdn;
  }
  else
  {
    p_otp_addr = OTP_Read(0);
    if (p_otp_addr)
    {
      p_bd_addr = ((OTP_ID0_t*)p_otp_addr)->bd_address;
    }
    else
    {
      p_bd_addr = a_MBdAddr;
    }
  }

  return p_bd_addr;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS */

/**
 * @brief Parse an Eddystone-TLM Service Data payload.
 *
 * service_data points to the first byte after the AD type:
 *
 *   [0]      Eddystone UUID LSB (0xAA)
 *   [1]      Eddystone UUID MSB (0xFE)
 *   [2]      Frame Type (0x20 = TLM)
 *   [3]      TLM Version
 *   [4..5]   Battery voltage, big endian, mV
 *   [6..7]   Temperature, signed 8.8 fixed point, big endian
 *   [8..11]  Advertisement count, big endian
 *   [12..15] Seconds since boot, 0.1 s units, big endian
 */
static void APP_BLE_Parse_Eddystone_TLM(uint8_t *service_data,
		uint8_t service_data_len) {
	uint16_t humidity_raw;
	uint16_t temperature_raw;
	uint32_t adv_count;
	uint32_t time_since_boot;

	if (service_data == NULL || service_data_len < 16U) {
		return;
	}

	/*
	 * Eddystone UUID = 0xFEAA
	 *
	 * In the received advertising data:
	 *   service_data[0] = 0xAA
	 *   service_data[1] = 0xFE
	 */
	if ((service_data[0] != 0xAAU) || (service_data[1] != 0xFEU)) {
		return;
	}

	/*
	 * Eddystone-TLM frame type = 0x20
	 */
	if (service_data[2] != EDDYSTONE_FRAME_TLM) {
		return;
	}

	/*
	 * ---------------------------------------------------------
	 * RAW SHT41 HUMIDITY
	 *
	 * Transmitter:
	 *
	 *   service_data[4] = raw_humidity MSB
	 *   service_data[5] = raw_humidity LSB
	 *
	 * Reconstruct 16-bit value.
	 * ---------------------------------------------------------
	 */
	humidity_raw = ((uint16_t) service_data[4] << 8)
			| (uint16_t) service_data[5];

	/*
	 * ---------------------------------------------------------
	 * RAW SHT41 TEMPERATURE
	 *
	 * Transmitter:
	 *
	 *   service_data[6] = raw_temperature MSB
	 *   service_data[7] = raw_temperature LSB
	 *
	 * Reconstruct 16-bit value.
	 * ---------------------------------------------------------
	 */
	temperature_raw = ((uint16_t) service_data[6] << 8)
			| (uint16_t) service_data[7];

	/*
	 * ---------------------------------------------------------
	 * Advertisement count
	 * ---------------------------------------------------------
	 */
	adv_count = ((uint32_t) service_data[8] << 24)
			| ((uint32_t) service_data[9] << 16)
			| ((uint32_t) service_data[10] << 8) | (uint32_t) service_data[11];

	/*
	 * ---------------------------------------------------------
	 * Time since boot
	 * ---------------------------------------------------------
	 */
	time_since_boot = ((uint32_t) service_data[12] << 24)
			| ((uint32_t) service_data[13] << 16)
			| ((uint32_t) service_data[14] << 8) | (uint32_t) service_data[15];

	/*
	 * Store newest complete packet
	 */
	g_eddystone_tlm.version = service_data[3];

	/*
	 * These fields are being repurposed:
	 *
	 * battery_mv     -> RAW SHT41 humidity
	 * temperature_raw -> RAW SHT41 temperature
	 */
	g_eddystone_tlm.battery_mv = humidity_raw;
	g_eddystone_tlm.temperature_raw = temperature_raw;

	g_eddystone_tlm.adv_count = adv_count;
	g_eddystone_tlm.time_since_boot = time_since_boot;

	g_eddystone_tlm.valid = 1U;

	/*
	 * ---------------------------------------------------------
	 * DEBUG OUTPUT
	 * ---------------------------------------------------------
	 */
	APP_DBG_MSG("\r\n========== EDDYSTONE TLM ==========\r\n");

	APP_DBG_MSG("TLM Version: %u\r\n", (unsigned int) service_data[3]);

	APP_DBG_MSG("RAW Humidity: 0x%04X (%u)\r\n", (unsigned int) humidity_raw,
			(unsigned int) humidity_raw);

	APP_DBG_MSG("RAW Temperature: 0x%04X (%u)\r\n",
			(unsigned int) temperature_raw, (unsigned int) temperature_raw);

	APP_DBG_MSG("Advertisement Count: %lu\r\n", (unsigned long) adv_count);

	APP_DBG_MSG("Time Since Boot: %lu.%lu seconds\r\n",
			(unsigned long) (time_since_boot / 10U),
			(unsigned long) (time_since_boot % 10U));

	APP_DBG_MSG("===================================\r\n");
}

void APP_BLE_Get_Eddystone_TLM(Eddystone_TLM_Data_t *data)
{
  if (data == NULL || !g_scan_enabled)
  {
	data->valid = 0U;
    return;
  }

  data->valid = g_eddystone_tlm.valid;
  data->version = g_eddystone_tlm.version;
  data->battery_mv = g_eddystone_tlm.battery_mv;
  data->temperature_raw = g_eddystone_tlm.temperature_raw;
  data->adv_count = g_eddystone_tlm.adv_count;
  data->time_since_boot = g_eddystone_tlm.time_since_boot;
}


/* USER CODE END FD_LOCAL_FUNCTIONS */

/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/
void hci_notify_asynch_evt(void* pdata)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
  return;
}

void hci_cmd_resp_release(uint32_t flag)
{
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
  return;
}

void hci_cmd_resp_wait(uint32_t timeout)
{
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
  return;
}

static void BLE_UserEvtRx(void * pPayload)
{
  SVCCTL_UserEvtFlowStatus_t svctl_return_status;
  tHCI_UserEvtRxParam *pParam;

  pParam = (tHCI_UserEvtRxParam *)pPayload;

  svctl_return_status = SVCCTL_UserEvtRx((void *)&(pParam->pckt->evtserial));
  if (svctl_return_status != SVCCTL_UserEvtFlowDisable)
  {
    pParam->status = HCI_TL_UserEventFlow_Enable;
  }
  else
  {
    pParam->status = HCI_TL_UserEventFlow_Disable;
  }

  return;
}

static void BLE_StatusNot(HCI_TL_CmdStatus_t status)
{
  uint32_t task_id_list;
  switch (status)
  {
    case HCI_TL_CmdBusy:
      /**
       * All tasks that may send an aci/hci commands shall be listed here
       * This is to prevent a new command is sent while one is already pending
       */
      task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
      UTIL_SEQ_PauseTask(task_id_list);
      break;

    case HCI_TL_CmdAvailable:
      /**
       * All tasks that may send an aci/hci commands shall be listed here
       * This is to prevent a new command is sent while one is already pending
       */
      task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
      UTIL_SEQ_ResumeTask(task_id_list);
      break;

    default:
      break;
  }
  return;
}

void SVCCTL_ResumeUserEventFlow(void)
{
  hci_resume_flow();
  return;
}

/* USER CODE BEGIN FD_WRAP_FUNCTIONS */

/* USER CODE END FD_WRAP_FUNCTIONS */
