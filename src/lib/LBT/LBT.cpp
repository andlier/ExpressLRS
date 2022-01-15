#include "common.h"
#include "LBT.h"

extern SX1280Driver Radio;

LQCALC<100> LBTSuccessCalc;
volatile uint32_t rxStartTime;

#if !defined(LBT_RSSI_THRESHOLD_OFFSET_DB)
  #define LBT_RSSI_THRESHOLD_OFFSET_DB 0
#endif

bool LBTEnabled;

int ICACHE_RAM_ATTR SpreadingFactorToRSSIvalidDelayUs(SX1280_RadioLoRaSpreadingFactors_t SF)
{
  // The necessary wait time from RX start to valid instant RSSI reading
  // changes with the spreading factor setting.
  // The worst case necessary wait time is TX->RX switch time + Lora symbol time
  // This assumes the radio switches from either TX, RX or FS (Freq Synth mode)
  // TX to RX switch time is 60us for sx1280
  // Lora symbol time for the relevant spreading factors is:
  // SF5: 39.4us
  // SF6: 78.8us
  // SF7: 157.6us
  // SF9: 630.5us
  // However, by measuring when the RSSI reading is stable and valid, it was found that
  // actual necessary wait times are:
  // SF5 ~100us (60us + SF5 symbol time)
  // SF6 ~141us (60us + SF6 symbol time)
  // SF7 ~218us (60us + SF7 symbol time)
  // SF9 ~218us (Odd one out, measured to same as SF7 wait time)

  switch(SF)
    {
      case SX1280_LORA_SF5: return 100;
      case SX1280_LORA_SF6: return 141;
      case SX1280_LORA_SF7: return 218;
      case SX1280_LORA_SF9: return 218;
      default: return 218; // Values above 100mW are not relevant, default to 100mW threshold
    }
}

int8_t ICACHE_RAM_ATTR PowerEnumToLBTLimit(PowerLevels_e txPower)
{
  // Calculated from EN 300 328, adjusted for 800kHz BW for sx1280
  // TL = -70 dBm/MHz + 10*log10(0.8MHz) + 10 × log10 (100 mW / Pout) (Pout in mW e.i.r.p.)
  // This threshold should be offset with a #define config for each HW that corrects for antenna gain,
  // different RF frontends.
  // TODO: Maybe individual adjustment offset for differences in
  // rssi reading between bandwidth setting is also necessary when other BW than 0.8MHz are used.

  switch(txPower)
  {
    case PWR_10mW: return -61 + LBT_RSSI_THRESHOLD_OFFSET_DB;
    case PWR_25mW: return -65 + LBT_RSSI_THRESHOLD_OFFSET_DB;
    case PWR_50mW: return -68 + LBT_RSSI_THRESHOLD_OFFSET_DB;
    case PWR_100mW: return -71 + LBT_RSSI_THRESHOLD_OFFSET_DB;
    // Values above 100mW are not relevant, default to 100mW threshold
    default: return -71 + LBT_RSSI_THRESHOLD_OFFSET_DB;
  }
}

void ICACHE_RAM_ATTR BeginClearChannelAssessment(void)
{
  // Listen Before Talk (LBT) aka clear channel assessment (CCA)
  // Not interested in packets or interrupts while measuring RF energy on channel.
  Radio.SetDioIrqParams(SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
  if(Radio.GetMode() != SX1280_MODE_RX)
  {
    Radio.RXnb();
    rxStartTime = micros();
  }
}

bool ICACHE_RAM_ATTR ChannelIsClear(void)
{ 
  LBTSuccessCalc.inc(); // Increment count for every channel check

  // Read rssi after waiting the minimum RSSI valid delay.
  // If this function is called long enough after RX enable,
  // this will always be ok on first try as is the case for TX.

  // TODO: Better way than busypolling this for RX?
  // this loop should only run for RX, where listen before talk RX is started right after FHSS hop
  // so there is no dead-time to run RX before telemetry TX is supposed to happen.
  // if flipping the logic, so telemetry TX happens right before FHSS hop, then TX-side ends up with polling here instead?
  // Maybe it could be skipped if there was only TX of telemetry happening when FHSShop does not happen.
  // Then RX for LBT could stay enabled from last received packet, and RSSI would be valid instantly.
  // But for now, FHSShops and telemetry rates does not divide evenly, so telemetry will some times happen
  // right after FHSS and we need wait here.

  int validRSSIdelayUs = SpreadingFactorToRSSIvalidDelayUs((SX1280_RadioLoRaSpreadingFactors_t)ExpressLRS_currAirRate_Modparams->sf);
  int delayRemaining = validRSSIdelayUs - (micros() - rxStartTime);
  if(delayRemaining > validRSSIdelayUs)
  {
    // delayMicroseconds() only valid up to ~16ms per arduino spec. limit to max wait time we need just in case.
    delayRemaining = validRSSIdelayUs;
  }
  if(delayRemaining > 0) 
  {
    delayMicroseconds(delayRemaining);
  }

  return Radio.GetRssiInst() < PowerEnumToLBTLimit((PowerLevels_e)POWERMGNT::currPower());
}

void ICACHE_RAM_ATTR PrepareTXafterClearChannelAssessment(void)
{
  LBTSuccessCalc.add(); // Add success only when actually preparing for TX
  Radio.ClearIrqStatus(SX1280_IRQ_RADIO_ALL);
  Radio.SetDioIrqParams(SX1280_IRQ_TX_DONE, SX1280_IRQ_TX_DONE, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
}

void ICACHE_RAM_ATTR PrepareRXafterClearChannelAssessment(void)
{
  // Go to idle and back to rx, to prevent packet reception during LBT filling the RX buffer
  Radio.SetTxIdleMode();
  Radio.ClearIrqStatus(SX1280_IRQ_RADIO_ALL);
  Radio.SetDioIrqParams(SX1280_IRQ_RX_DONE, SX1280_IRQ_RX_DONE, SX1280_IRQ_RADIO_NONE, SX1280_IRQ_RADIO_NONE);
}
