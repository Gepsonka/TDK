use aes_gcm::Nonce;
use log::{debug, info, warn};
use rppal::gpio::{Gpio, InputPin, OutputPin, Trigger};
use rppal::spi::{Bus, Mode, Segment, SlaveSelect, Spi};
use std::error::Error;
use std::{thread, time};

#[allow(dead_code)]
pub const REG_FIFO: u8 = 0x00;
#[allow(dead_code)]
pub const REG_OP_MODE: u8 = 0x01;
#[allow(dead_code)]
pub const REG_BITRATE_MSB: u8 = 0x02;
#[allow(dead_code)]
pub const REG_FDEV_MSB: u8 = 0x04;
#[allow(dead_code)]
pub const REG_FRF_MSB: u8 = 0x06;
#[allow(dead_code)]
pub const REG_FRF_MID: u8 = 0x07;
#[allow(dead_code)]
pub const REG_FRF_LSB: u8 = 0x08;
#[allow(dead_code)]
pub const REG_PA_CONFIG: u8 = 0x09;
#[allow(dead_code)]
pub const REG_PA_RAMP: u8 = 0x0a;
#[allow(dead_code)]
pub const REG_OCP: u8 = 0x0b;
#[allow(dead_code)]
pub const REG_LNA: u8 = 0x0c;
#[allow(dead_code)]
pub const REG_FIFO_ADDR_PTR: u8 = 0x0d;
#[allow(dead_code)]
pub const REG_RX_CONFIG: u8 = 0x0d;
#[allow(dead_code)]
pub const REG_FIFO_TX_BASE_ADDR: u8 = 0x0e;
#[allow(dead_code)]
pub const REG_RSSI_CONFIG: u8 = 0x0e;
#[allow(dead_code)]
pub const REG_FIFO_RX_BASE_ADDR: u8 = 0x0f;
#[allow(dead_code)]
pub const REG_RSSI_COLLISION: u8 = 0x0f;
#[allow(dead_code)]
pub const REG_FIFO_RX_CURRENT_ADDR: u8 = 0x10;
#[allow(dead_code)]
pub const REG_RSSI_VALUE_FSK: u8 = 0x11;
#[allow(dead_code)]
pub const REG_IRQ_FLAGS: u8 = 0x12;
#[allow(dead_code)]
pub const REG_RX_BW: u8 = 0x12;
#[allow(dead_code)]
pub const REG_RX_NB_BYTES: u8 = 0x13;
#[allow(dead_code)]
pub const REG_AFC_BW: u8 = 0x13;
#[allow(dead_code)]
pub const REG_OOK_PEAK: u8 = 0x14;
#[allow(dead_code)]
pub const REG_OOK_FIX: u8 = 0x15;
#[allow(dead_code)]
pub const REG_OOK_AVG: u8 = 0x16;
#[allow(dead_code)]
pub const REG_PKT_SNR_VALUE: u8 = 0x19;
#[allow(dead_code)]
pub const REG_PKT_RSSI_VALUE: u8 = 0x1a;
#[allow(dead_code)]
pub const REG_RSSI_VALUE: u8 = 0x1b;
#[allow(dead_code)]
pub const REG_AFC_VALUE: u8 = 0x1b;
#[allow(dead_code)]
pub const REG_MODEM_CONFIG_1: u8 = 0x1d;
#[allow(dead_code)]
pub const REG_MODEM_CONFIG_2: u8 = 0x1e;
#[allow(dead_code)]
pub const REG_PREAMBLE_DETECT: u8 = 0x1f;
#[allow(dead_code)]
pub const REG_FEI_MSB: u8 = 0x1d;
#[allow(dead_code)]
pub const REG_PREAMBLE_MSB: u8 = 0x20;
#[allow(dead_code)]
pub const REG_PREAMBLE_LSB: u8 = 0x21;
#[allow(dead_code)]
pub const REG_PREAMBLE_MSB_FSK: u8 = 0x25;
#[allow(dead_code)]
pub const REG_PAYLOAD_LENGTH: u8 = 0x22;
#[allow(dead_code)]
pub const REG_MODEM_CONFIG_3: u8 = 0x26;
#[allow(dead_code)]
pub const REG_SYNC_CONFIG: u8 = 0x27;
#[allow(dead_code)]
pub const REG_FREQ_ERROR_MSB: u8 = 0x28;
#[allow(dead_code)]
pub const REG_SYNC_VALUE1: u8 = 0x28;
#[allow(dead_code)]
pub const REG_FREQ_ERROR_MID: u8 = 0x29;
#[allow(dead_code)]
pub const REG_FREQ_ERROR_LSB: u8 = 0x2a;
#[allow(dead_code)]
pub const REG_RSSI_WIDEBAND: u8 = 0x2c;
#[allow(dead_code)]
pub const REG_PACKET_CONFIG1: u8 = 0x30;
#[allow(dead_code)]
pub const REG_DETECTION_OPTIMIZE: u8 = 0x31;
#[allow(dead_code)]
pub const REG_PACKET_CONFIG2: u8 = 0x31;
#[allow(dead_code)]
pub const REG_PAYLOAD_LENGTH_FSK: u8 = 0x32;
#[allow(dead_code)]
pub const REG_INVERTIQ: u8 = 0x33;
#[allow(dead_code)]
pub const REG_NODE_ADDR: u8 = 0x33;
#[allow(dead_code)]
pub const REG_BROADCAST_ADDR: u8 = 0x34;
#[allow(dead_code)]
pub const REG_FIFO_THRESH: u8 = 0x35;
#[allow(dead_code)]
pub const REG_SEQ_CONFIG1: u8 = 0x36;
#[allow(dead_code)]
pub const REG_DETECTION_THRESHOLD: u8 = 0x37;
#[allow(dead_code)]
pub const REG_TIMER_RESOLUTION: u8 = 0x38;
#[allow(dead_code)]
pub const REG_TIMER1_COEF: u8 = 0x39;
#[allow(dead_code)]
pub const REG_TIMER2_COEF: u8 = 0x3a;
#[allow(dead_code)]
pub const REG_SYNC_WORD: u8 = 0x39;
#[allow(dead_code)]
pub const REG_INVERTIQ2: u8 = 0x3b;
#[allow(dead_code)]
pub const REG_IMAGE_CAL: u8 = 0x3b;
#[allow(dead_code)]
pub const REG_IRQ_FLAGS_1: u8 = 0x3e;
#[allow(dead_code)]
pub const REG_IRQ_FLAGS_2: u8 = 0x3f;
#[allow(dead_code)]
pub const REG_DIO_MAPPING_1: u8 = 0x40;
#[allow(dead_code)]
pub const REG_DIO_MAPPING_2: u8 = 0x41;
#[allow(dead_code)]
pub const REG_VERSION: u8 = 0x42;
#[allow(dead_code)]
pub const REG_PA_DAC: u8 = 0x4d;
#[allow(dead_code)]
pub const REG_BITRATE_FRAC: u8 = 0x5d;

#[allow(dead_code)]
pub const SX127x_VERSION: u8 = 0x12;

pub const SX127x_DIO0_RX_DONE: u8 = 0b00000000; // Packet reception complete
pub const SX127x_DIO0_TX_DONE: u8 = 0b01000000; // FIFO Payload transmission complete
pub const SX127x_DIO0_CAD_DONE: u8 = 0b10000000; // CAD complete
pub const SX127x_DIO1_RXTIMEOUT: u8 = 0b00000000; // RX timeout interrupt. Used in RX single mode
pub const SX127x_DIO1_FHSS_CHANGE_CHANNEL: u8 = 0b00010000; // FHSS change channel
pub const SX127x_DIO1_CAD_DETECTED: u8 = 0b00100000; // Valid Lora signal detected during CAD operation
pub const SX127x_DIO2_FHSS_CHANGE_CHANNEL: u8 = 0b00000000; // FHSS change channel on digital pin 2
pub const SX127x_DIO3_CAD_DONE: u8 = 0b00000000; // CAD complete on digital pin 3
pub const SX127x_DIO3_VALID_HEADER: u8 = 0b00000001; // Valid header received in Rx
pub const SX127x_DIO3_PAYLOAD_CRC_ERROR: u8 = 0b00000010; // Payload CRC error

pub const SX127x_OSCILLATOR_FREQUENCY: f32 = 32000000.0f32;
pub const SX127x_FREQ_ERROR_FACTOR: f32 = ((1 << 24) as f32 / SX127x_OSCILLATOR_FREQUENCY);
pub const SX127x_FSTEP: f32 = (SX127x_OSCILLATOR_FREQUENCY / (1 << 19) as f32);
pub const SX127x_REG_MODEM_CONFIG_3_AGC_ON: u8 = 0b00000100;
pub const SX127x_REG_MODEM_CONFIG_3_AGC_OFF: u8 = 0b00000000;

pub const SX127x_IRQ_FLAG_RXTIMEOUT: u8 = 0b10000000;
pub const SX127x_IRQ_FLAG_RXDONE: u8 = 0b01000000;
pub const SX127x_IRQ_FLAG_PAYLOAD_CRC_ERROR: u8 = 0b00100000;
pub const SX127x_IRQ_FLAG_VALID_HEADER: u8 = 0b00010000;
pub const SX127x_IRQ_FLAG_TXDONE: u8 = 0b00001000;
pub const SX127x_IRQ_FLAG_CADDONE: u8 = 0b00000100;
pub const SX127x_IRQ_FLAG_FHSSCHANGECHANNEL: u8 = 0b00000010;
pub const SX127x_IRQ_FLAG_CAD_DETECTED: u8 = 0b00000001;

pub const SX127X_FSK_IRQ_FIFO_FULL: u8 = 0b10000000;
pub const SX127X_FSK_IRQ_FIFO_EMPTY: u8 = 0b01000000;
pub const SX127X_FSK_IRQ_FIFO_LEVEL: u8 = 0b00100000;
pub const SX127X_FSK_IRQ_FIFO_OVERRUN: u8 = 0b00010000;
pub const SX127X_FSK_IRQ_PACKET_SENT: u8 = 0b00001000;
pub const SX127X_FSK_IRQ_PAYLOAD_READY: u8 = 0b00000100;
pub const SX127X_FSK_IRQ_CRC_OK: u8 = 0b00000010;
pub const SX127X_FSK_IRQ_LOW_BATTERY: u8 = 0b00000001;
pub const SX127X_FSK_IRQ_PREAMBLE_DETECT: u8 = 0b00000010;
pub const SX127X_FSK_IRQ_SYNC_ADDRESS_MATCH: u8 = 0b00000001;

pub const FIFO_TX_BASE_ADDR: u8 = 0b00000000;
pub const FIFO_RX_BASE_ADDR: u8 = 0b00000000;

pub const RF_MID_BAND_THRESHOLD: u32 = 525000000;
pub const RSSI_OFFSET_HF_PORT: u8 = 157;
pub const RSSI_OFFSET_LF_PORT: u8 = 164;

pub const SX127x_MAX_POWER: u8 = 0b01110000;
pub const SX127x_LOW_POWER: u8 = 0b00000000;

pub const SX127x_HIGH_POWER_ON: u8 = 0b10000111;
pub const SX127x_HIGH_POWER_OFF: u8 = 0b10000100;

#[allow(dead_code)]
#[derive(Clone, Copy, Eq, PartialEq)]
pub enum Bandwidth {
    SX127x_BW_7800 = 0b00000000,
    SX127x_BW_10400 = 0b00010000,
    SX127x_BW_15600 = 0b00100000,
    SX127x_BW_20800 = 0b00110000,
    SX127x_BW_31250 = 0b01000000,
    SX127x_BW_41700 = 0b01010000,
    SX127x_BW_62500 = 0b01100000,
    SX127x_BW_125000 = 0b01110000, // default
    SX127x_BW_250000 = 0b10000000,
    SX127x_BW_500000 = 0b10010000,
}

#[allow(dead_code)]
#[derive(Clone, Copy, Eq, PartialEq, Debug)]
pub enum OperationMode {
    SX127x_MODE_SLEEP = 0b00000000,     // SLEEP
    SX127x_MODE_STANDBY = 0b00000001,   // STDBY
    SX127x_MODE_FSTX = 0b00000010,      // Frequency synthesis TX
    SX127x_MODE_TX = 0b00000011,        // Transmit
    SX127x_MODE_FSRX = 0b00000100,      // Frequency synthesis RX
    SX127x_MODE_RX_CONT = 0b00000101,   // Receive continuous
    SX127x_MODE_RX_SINGLE = 0b00000110, // Receive single
    SX127x_MODE_CAD = 0b00000111,       // Channel activity detection
}

#[allow(dead_code)]
#[derive(Clone, Copy, Eq, PartialEq)]
pub enum SpreadFactor {
    SX127x_SF_6 = 0b01100000,  // 64 chips / symbol
    SX127x_SF_7 = 0b01110000,  // 128 chips / symbol
    SX127x_SF_8 = 0b10000000,  // 256 chips / symbol
    SX127x_SF_9 = 0b10010000,  // 512 chips / symbol
    SX127x_SF_10 = 0b10100000, // 1024 chips / symbol
    SX127x_SF_11 = 0b10110000, // 2048 chips / symbol
    SX127x_SF_12 = 0b11000000, // 4096 chips / symbol
}

#[allow(dead_code)]
pub enum Modulation {
    SX127x_MODULATION_LORA = 0b10000000,
    SX127x_MODULATION_FSK = 0b00000000, // default
    SX127x_MODULATION_OOK = 0b00100000,
} // for now we will only use lora

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum HeaderMode {
    Implicit = 0b00000000,
    Explicit = 0b00000001,
}

#[allow(dead_code)]
pub struct LoRa {
    spi: Spi,
    reset_pin: OutputPin,
    header_mode: HeaderMode,
    frequency: u64,
    pub opmod: Option<OperationMode>,
    spread_factor: Option<SpreadFactor>,
    rx_callback: fn(Vec<u8>),
    tx_callback: fn(),
    pub waiting_for_tx: bool,
}

impl LoRa {
    pub fn new(
        spi: Spi,
        reset_pin: OutputPin,
        header_mode: HeaderMode,
        frequency: u64,
        rx_callback: fn(Vec<u8>),
        tx_callback: fn(),
    ) -> Result<Self, Box<dyn Error>> {
        let mut lora = LoRa {
            spi,
            reset_pin,
            header_mode,
            opmod: None,
            spread_factor: None,
            frequency,
            rx_callback,
            tx_callback,
            waiting_for_tx: false,
        };

        Ok(lora)
    }

    fn read_register(
        &mut self,
        register: u8,
        buff: &mut [u8],
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.spi
            .transfer_segments(&[Segment::with_write(&[register]), Segment::with_read(buff)])?;
        Ok(())
    }

    pub fn write_register(
        &mut self,
        register: u8,
        buff: &[u8],
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.spi.transfer_segments(&[
            Segment::with_write(&[0b10000000 | register]),
            Segment::with_write(buff),
        ])?;
        Ok(())
    }

    pub fn write_single(
        &mut self,
        register: u8,
        value: u8,
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.spi.write(&[register, value])?;
        Ok(())
    }

    pub fn append_register(
        &mut self,
        register: u8,
        value: u8,
        mask: u8,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let mut prev: [u8; 1] = [0];
        self.read_register(register, &mut prev)?;
        let mut data: [u8; 1] = [((prev[0] & mask) | value)];
        self.write_register(register, &data)?;

        Ok(())
    }

    pub fn reset(&mut self) {
        self.reset_pin.set_low();
        thread::sleep(time::Duration::from_millis(200));
        self.reset_pin.set_high();
        thread::sleep(time::Duration::from_millis(10));
        info!("LoRa reset done.");
    }

    pub fn check_version(&mut self) -> Result<u8, Box<dyn std::error::Error>> {
        let mut buff: [u8; 1] = [0];
        self.read_register(REG_VERSION, &mut buff)?;
        if buff[0] != SX127x_VERSION {
            panic!("LoRa version mismatch: {:#x}", buff[0]);
        }
        Ok(buff[0])
    }

    pub fn sleep(&mut self) -> Result<(), Box<dyn Error>> {
        self.write_register(REG_OP_MODE, &[0x80, 0x00])?;
        Ok(())
    }

    pub fn set_opmod(&mut self, opmod: OperationMode) -> Result<(), Box<dyn std::error::Error>> {
        if opmod == OperationMode::SX127x_MODE_RX_CONT
            || opmod == OperationMode::SX127x_MODE_RX_SINGLE
        {
            self.append_register(REG_DIO_MAPPING_1, SX127x_DIO0_RX_DONE, 0b00111111)?;
        } else if opmod == OperationMode::SX127x_MODE_TX {
            self.append_register(REG_DIO_MAPPING_1, SX127x_DIO0_TX_DONE, 0b00111111)?;
        } else if opmod == OperationMode::SX127x_MODE_CAD {
            self.append_register(REG_DIO_MAPPING_1, SX127x_DIO0_CAD_DONE, 0b00111111)?;
        }

        let value: u8 = (Modulation::SX127x_MODULATION_LORA as u8 | opmod as u8);
        self.write_register(REG_OP_MODE, &[value])?;
        self.opmod = Some(opmod);
        Ok(())
    }

    pub fn get_opmod(&mut self) -> Result<u8, Box<dyn Error>> {
        let mut buff: [u8; 1] = [0];
        self.read_register(REG_OP_MODE, &mut buff)?;
        Ok(buff[0])
    }

    pub fn set_frequency(&mut self, freq: u64) -> Result<(), Box<dyn std::error::Error>> {
        let adjusted: u64 = (freq << 19) / SX127x_OSCILLATOR_FREQUENCY as u64;
        let data: [u8; 3] = [
            ((adjusted >> 16) & 0xff) as u8,
            ((adjusted >> 8) & 0xff) as u8,
            (adjusted & 0xff) as u8,
        ];
        self.write_register(REG_FRF_MSB, &data)?;
        self.frequency = freq;
        info!("Frequency set to: {}", freq);
        Ok(())
    }

    pub fn reload_low_datarate_optimalization(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        let mut bandwidth: [u32; 1] = [self.get_bandwidth()?];
        bandwidth[0] = self.get_bandwidth()?;
        let mut spreading_factor: [u8; 1] = [0];
        self.read_register(REG_MODEM_CONFIG_2, &mut spreading_factor)?;
        spreading_factor[0] = spreading_factor[0] >> 4;
        let mut symbol_duration: u32 = 1000 / (bandwidth[0] / (1u32 << spreading_factor[0]));
        if symbol_duration > 16 {
            // force low data rate optimization
            self.set_low_datarate_optimization(true)?;
        }
        Ok(())
    }

    pub fn set_low_datarate_optimization(
        &mut self,
        enable: bool,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let mut value: u8;
        if enable {
            value = 0b00001000;
        } else {
            value = 0b00000000;
        }

        self.append_register(REG_MODEM_CONFIG_3, value, 0b11110111)?;

        Ok(())
    }

    pub fn get_bandwidth(&mut self) -> Result<u32, Box<dyn std::error::Error>> {
        let mut buff: [u8; 1] = [0];
        self.read_register(REG_MODEM_CONFIG_1, &mut buff)?;
        buff[0] = buff[0] >> 4;
        let bw = match buff[0] {
            0b0000 => 7800,
            0b0001 => 10400,
            0b0010 => 15600,
            0b0011 => 20800,
            0b0100 => 31250,
            0b0101 => 41700,
            0b0110 => 62500,
            0b0111 => 125000,
            0b1000 => 250000,
            0b1001 => 500000,
            _ => Err("Invalid bandwidth")?,
        };

        Ok(bw)
    }

    pub fn set_bandwidth(
        &mut self,
        bandwidth: Bandwidth,
    ) -> Result<(), Box<dyn std::error::Error>> {
        self.append_register(REG_MODEM_CONFIG_1, bandwidth as u8, 0b00001111)?;
        self.reload_low_datarate_optimalization()?;
        info!("LoRa bandwidth set to: {}", bandwidth as u32);
        Ok(())
    }

    pub fn reset_fifo(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        let data: [u8; 2] = [FIFO_TX_BASE_ADDR, FIFO_RX_BASE_ADDR];
        self.write_register(REG_FIFO_TX_BASE_ADDR, &data)?;
        info!("LoRa fifo reset done.");
        Ok(())
    }

    pub fn rx_set_lna_boost_hf(&mut self, enable: bool) -> Result<(), Box<dyn std::error::Error>> {
        let mut value: u8;
        if enable {
            value = 0b00000011;
        } else {
            value = 0b00000000;
        }

        self.append_register(REG_LNA, value, 0b11111100)?;
        Ok(())
    }

    pub fn set_header_type(
        &mut self,
        header_mode: HeaderMode,
    ) -> Result<(), Box<dyn std::error::Error>> {
        if header_mode == HeaderMode::Explicit {
            self.append_register(REG_MODEM_CONFIG_1, HeaderMode::Explicit as u8, 0b11111110)?;
            info!("LoRa header mode: Explicit");
        } else {
            todo!()
        }
        Ok(())
    }

    pub fn set_modem_config_2(
        &mut self,
        sf: SpreadFactor,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let detection_optimize: u8;
        let detection_threshold: u8;
        if sf == SpreadFactor::SX127x_SF_6 {
            detection_optimize = 0xc5;
            detection_threshold = 0x0c;
        } else {
            detection_optimize = 0xc3;
            detection_threshold = 0x0a;
        }

        self.write_register(REG_DETECTION_OPTIMIZE, &[detection_optimize])?;
        self.write_register(REG_DETECTION_THRESHOLD, &[detection_threshold])?;
        self.append_register(REG_MODEM_CONFIG_2, sf as u8, 0b00001111)?;
        self.reload_low_datarate_optimalization()?;
        self.spread_factor = Some(sf);
        info!("Spread factor set to: {}", sf as u8);

        Ok(())
    }

    pub fn set_syncword(&mut self, value: u8) -> Result<(), Box<dyn std::error::Error>> {
        self.write_register(REG_SYNC_WORD, &[value])?;
        info!("Syncword set to: {:x}", value);
        Ok(())
    }

    pub fn set_preamble_length(&mut self, value: u16) -> Result<(), Box<dyn std::error::Error>> {
        let data: [u8; 2] = [((value >> 8) & 0xff) as u8, (value & 0xff) as u8];
        self.write_register(REG_PREAMBLE_MSB, &data)?;
        info!("Preamble length set to: {}", value);
        Ok(())
    }

    pub fn get_packet_rssi(&mut self) -> Result<i16, Box<dyn std::error::Error>> {
        let mut buff: [u8; 1] = [0];
        let mut rssi: i16;
        self.read_register(REG_PKT_RSSI_VALUE, &mut buff)?;
        if self.frequency < RF_MID_BAND_THRESHOLD as u64 {
            rssi = (buff[0] - RSSI_OFFSET_LF_PORT as u8) as i16;
        } else {
            rssi = (buff[0] - RSSI_OFFSET_HF_PORT as u8) as i16;
        }

        let mut snr = self.get_packet_snr();
        match snr {
            Ok(snr) => Ok((rssi + (snr as i16))),
            Err(_) => Ok(0),
        }
    }

    pub fn get_packet_snr(&mut self) -> Result<f32, Box<dyn std::error::Error>> {
        let mut buff: [u8; 1] = [0];
        let mut snr: f32;
        self.read_register(REG_PKT_SNR_VALUE, &mut buff)?;
        snr = buff[0] as f32 * 0.25f32;

        Ok(snr)
    }

    pub fn get_frequency_error(&mut self) -> Result<i32, Box<dyn std::error::Error>> {
        let mut result: i32;
        let mut buff: [u8; 3] = [0; 3];
        self.read_register(REG_FREQ_ERROR_MSB, &mut buff)?;
        let mut freq_error: i32 =
            ((buff[0] as i32) << 16) | ((buff[1] as i32) << 8) | (buff[2] as i32);
        let mut bandwidth: u32 = self.get_bandwidth()?;
        if freq_error & 0x80000 != 0 {
            freq_error = (!(freq_error) + 1) & 0xfffff;
            result = -1;
        } else {
            result = 1;
        }

        Ok(result
            * (freq_error as f32 * SX127x_FREQ_ERROR_FACTOR * bandwidth as f32 / 500000.0f32)
                as i32)
    }

    pub fn read_payload(&mut self, buff: &mut [u8]) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
        let mut length: [u8; 1] = [0];
        self.read_register(REG_RX_NB_BYTES, &mut length)?;

        let mut current_addr: [u8; 1] = [0];
        self.read_register(REG_FIFO_RX_CURRENT_ADDR, &mut current_addr)?;
        self.write_register(REG_FIFO_ADDR_PTR, &[current_addr[0]])?;
        let mut payload_buff: Vec<u8> = vec![0; length[0] as usize];
        self.read_register(REG_FIFO, &mut payload_buff)?;

        Ok(payload_buff)
    }

    pub fn handle_interrupt(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        let mut irq_flags: [u8; 1] = [0];
        self.read_register(REG_IRQ_FLAGS, &mut irq_flags)?;
        self.write_register(REG_IRQ_FLAGS, &irq_flags)?;
        if irq_flags[0] & SX127x_IRQ_FLAG_CADDONE != 0 {
            todo!("CAD done");
        }
        if irq_flags[0] & SX127x_IRQ_FLAG_PAYLOAD_CRC_ERROR != 0 {
            todo!("Payload CRC error");
        }

        if irq_flags[0] & SX127x_IRQ_FLAG_RXDONE != 0 {
            let payload: Vec<u8> = self.read_payload(&mut [0; 255])?;
            (self.rx_callback)(payload);
        }

        if irq_flags[0] & SX127x_IRQ_FLAG_TXDONE != 0 {
            self.waiting_for_tx = false;
            (self.tx_callback)();
        }

        Ok(())
    }

    // Must immediately followed by a set_opmod call to start the transmission
    pub fn set_for_transmission(&mut self, data: &[u8]) -> Result<(), Box<dyn Error>> {
        if data.len() == 0 {
            Err("Data length must be greater than 0")?;
        }

        if data.len() > 255 {
            Err("Data length must be less than 255")?;
        }

        self.write_register(REG_FIFO_ADDR_PTR, &[FIFO_TX_BASE_ADDR])?;
        self.write_register(REG_PAYLOAD_LENGTH, &[data.len() as u8])?;
        self.write_register(REG_FIFO, data)?;

        self.waiting_for_tx = true;
        Ok(())
    }

    pub fn set_ppm_offset(&mut self, freq_err: i32) -> Result<(), Box<dyn Error>> {
        let value: u8 = (0.95f32 * (freq_err as f32 / (self.frequency as f32 / 1E6f32))) as u8;
        self.write_register(REG_FREQ_ERROR_LSB, &[value])?;

        Ok(())
    }
}
