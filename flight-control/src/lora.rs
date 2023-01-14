use aes_gcm::KeyInit;
use embedded_hal::spi::SpiDevice;
use esp_idf_hal::delay::FreeRtos;
use esp_idf_hal::gpio::{Pin, PinDriver, Input, Output};
use esp_idf_hal::spi::{self, Spi, SpiDriver, SpiDeviceDriver};
use esp_idf_hal::{gpio, prelude::*};
use esp_idf_sys::EspError;
// use embedded_hal::blocking::delay::{DelayMs, DelayUs};

const REG_FIFO: u8 = 0x00;
const REG_OP_MODE: u8 = 0x01;
const REG_FRF_MSB: u8 = 0x06;
const REG_FRF_MID: u8 = 0x07;
const REG_FRF_LSB: u8 = 0x08;
const REG_PA_CONFIG: u8 = 0x09;
const REG_PA_RAMP: u8 = 0x0A;
const REG_OCP: u8 = 0x0B;
const REG_LNA: u8 = 0x0C;
const REG_FIFO_ADDR_PTR: u8 = 0x0D;
const REG_FIFO_TX_BASE_ADDR: u8 = 0x0E;
const REG_FIFO_RX_BASE_ADDR: u8 = 0x0F;
const REG_FIFO_RX_CURRENT_ADDR: u8 = 0x10;
const REG_IRQ_FLAGS: u8 = 0x12;
const REG_RX_NB_BYTES: u8 = 0x13;
const REG_PKT_SNR_VALUE: u8 = 0x19;
const REG_PKT_RSSI_VALUE: u8 = 0x1A;
const REG_MODEM_CONFIG_1: u8 = 0x1D;
const REG_MODEM_CONFIG_2: u8 = 0x1E;
const REG_PREAMBLE_MSB: u8 = 0x20;
const REG_PREAMBLE_LSB: u8 = 0x21;
const REG_PAYLOAD_LENGTH: u8 = 0x22;
const REG_MODEM_CONFIG_3: u8 = 0x26;
const REG_RSSI_WIDEBAND: u8 = 0x2C;
const REG_DETECTION_OPTIMIZE: u8 = 0x31;
const REG_DETECTION_THRESHOLD: u8 = 0x37;
const REG_SYNC_WORD: u8 = 0x39;
const REG_DIO_MAPPING_1: u8 = 0x40;
const REG_VERSION: u8 = 0x42;

const MODE_LONG_RANGE_MODE: u8 = 0b1000;
const MODE_SLEEP: u8 = 0b0000;
const MODE_STDBY: u8 = 0b0010;
const MODE_TX: u8 = 0b0100;
const MODE_RX_CONTINUOUS: u8 = 0b0110;
const MODE_RX_SINGLE: u8 = 0b0111;

const PA_BOOST: u8 = 0b10000000;

const IRQ_TX_DONE_MASK: u8 = 0b10000000;
const IRQ_PAYLOAD_CRC_ERROR_MASK: u8 = 0b01000000;
const IRQ_RX_DONE_MASK: u8 = 0b00100000;

const MAX_PKT_LENGTH: usize = 255;

#[derive(Debug, Clone, Copy)]
pub enum Bandwidth {
    K7,
    K10,
    K15,
    K20,
    K31,
    K41,
    K62,
    K125,
    K250,
    K500,
}

impl Bandwidth {
    fn value(self) -> u8 {
        match self {
            Bandwidth::K7 => 0b0000,
            Bandwidth::K10 => 0b0001,
            Bandwidth::K15 => 0b0010,
            Bandwidth::K20 => 0b0011,
            Bandwidth::K31 => 0b0100,
            Bandwidth::K41 => 0b0101,
            Bandwidth::K62 => 0b0110,
            Bandwidth::K125 => 0b0111,
            Bandwidth::K250 => 0b1000,
            Bandwidth::K500 => 0b1001,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum CodingRate {
    K4,
    K8,
    K12,
    K16,
    K20,
    K24,
    K28,
}

impl CodingRate {
    fn value(self) -> u8 {
        match self {
            CodingRate::K4 => 0b1001,
            CodingRate::K8 => 0b1000,
            CodingRate::K12 => 0b0111,
            CodingRate::K16 => 0b0110,
            CodingRate::K20 => 0b0101,
            CodingRate::K24 => 0b0100,
            CodingRate::K28 => 0b0011,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum SpreadingFactor {
    SF6,
    SF7,
    SF8,
    SF9,
    SF10,
    SF11,
    SF12,
}

impl SpreadingFactor {
    fn value(self) -> u8 {
        match self {
            SpreadingFactor::SF6 => 0b0110,
            SpreadingFactor::SF7 => 0b0111,
            SpreadingFactor::SF8 => 0b1000,
            SpreadingFactor::SF9 => 0b1001,
            SpreadingFactor::SF10 => 0b1010,
            SpreadingFactor::SF11 => 0b1011,
            SpreadingFactor::SF12 => 0b1100,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct ModemConfig {
    pub bandwidth: Bandwidth,
    pub coding_rate: CodingRate,
    pub spreading_factor: SpreadingFactor,
    pub implicit_header: bool,
    pub rx_continuous: bool,
    pub crc_on: bool,
}

impl ModemConfig {
    fn to_reg_val(&self) -> u8 {
        let mut val = self.bandwidth.value() << 4;
        val |= self.coding_rate.value() << 1;
        val |= self.spreading_factor.value() >> 2;
        if self.implicit_header {
            val |= 0b10000000;
        }
        if self.rx_continuous {
            val |= 0b01000000;
        }
        if self.crc_on {
            val |= 0b00100000;
        }
        val
    }
}

#[derive(Debug, Clone, Copy)]
pub struct PktConfig {
    pub fixed_len: bool,
    pub crc_on: bool,
    pub addr_filtering: u8,
    pub pkt_format: u8,
}

impl PktConfig {
    fn to_reg_val(&self) -> u8 {
        let mut val = 0;
        if self.fixed_len {
            val |= 0b10000000;
        }
        if self.crc_on {
            val |= 0b01000000;
        }
        val |= (self.addr_filtering & 0b0011) << 5;
        val |= self.pkt_format & 0b00011111;
        val
    }
}

pub struct LoRa<'a, ResetPin, DIO0Pin, DIO1Pin>
where
    ResetPin: Pin,
    DIO0Pin: Pin,
    DIO1Pin: Pin,
{
    spi: SpiDeviceDriver<'a, SpiDriver<'a>>,
    reset: PinDriver<'a, ResetPin, Output>,
    dio0: PinDriver<'a, DIO0Pin, Input>,
    dio1: PinDriver<'a, DIO1Pin, Input>,
    frequency: u32,
    pub tx_in_progress: bool,
}



impl<'a, ResetPin, DIO0Pin, DIO1Pin> LoRa<'a, ResetPin, DIO0Pin, DIO1Pin>
where
    ResetPin: Pin,
    DIO0Pin: Pin,
    DIO1Pin: Pin,
{
    pub fn new(
        spi: SpiDeviceDriver<'a, SpiDriver<'a>>,
        reset: PinDriver<'a, ResetPin, Output>,
        dio0: PinDriver<'a, DIO0Pin, Input>,
        dio1: PinDriver<'a, DIO1Pin, Input>,
        frequency: u32,
    ) -> Self {
        LoRa {
            spi,
            reset,
            dio0,
            dio1,
            frequency,
            tx_in_progress: false,
        }
    }

    pub fn init(&mut self, modem_config: ModemConfig) -> Result<(), EspError> {
        self.reset();
        self.set_mode(MODE_STDBY)?;
        self.set_modem_config(modem_config)?;
        self.set_frequency(self.frequency)?;
        Ok(())
    }

    fn reset(&mut self) {
        self.reset.set_low();
        FreeRtos::delay_ms(10);
        self.reset.set_high().ok();
        FreeRtos::delay_ms(10);
    }

    fn set_mode(&mut self, mode: u8) -> Result<(), EspError> {
        let mut buf = [0];
        let mut receive_buf: [u8; 1]= [0];
        //self.cs.set_low().ok();
        buf[0] = mode | MODE_LONG_RANGE_MODE;
        self.spi
            .transfer(&mut receive_buf, &mut buf)?;
        
        Ok(())
    }

    fn set_modem_config(&mut self, config: ModemConfig) -> Result<(), EspError> {
        let mut buf = [0; 2];
        //self.cs.set_low().ok();
        let mut receive_buf: [u8; 1]= [0];
        buf[0] = REG_MODEM_CONFIG_1 | 0x80;
        buf[1] = config.to_reg_val();
        self.spi
            .transfer(&mut receive_buf, &mut buf)
    }

    fn set_frequency(&mut self, frequency: u32) -> Result<(), EspError> {
        let mut buf = [0; 4];
        //self.cs.set_low().ok();
        let mut receive_buf: [u8; 1]= [0];
        buf[0] = REG_FRF_MSB | 0x80;
        buf[1] = (frequency >> 16) as u8;
        buf[2] = (frequency >> 8) as u8;
        buf[3] = frequency as u8;
        self.spi
            .transfer(&mut receive_buf, &mut buf)
    }

    pub fn send(&mut self, data: &[u8]) -> Result<(), EspError> {
        self.set_mode(MODE_STDBY)?;
        self.write_reg(REG_FIFO_ADDR_PTR, &[0])?;
        self.write_reg(REG_PAYLOAD_LENGTH, &[data.len() as u8])?;
        self.write_reg(REG_FIFO, &data)?;
        self.set_mode(MODE_TX)?;
        self.tx_in_progress = true;
        Ok(())
    }

    pub fn receive_packet(&mut self, timeout: u8) -> Result<Vec<u8>, ()> {
        self.set_mode(MODE_RX_SINGLE).ok();
        let mut buf = [0; 2];
        let mut receive_buf: [u8; 1]= [0];
        let mut data = Vec::new();
        let mut done = false;
        let mut t = 0;
        while !done && t < timeout {
            if self.dio0.is_high() {
                //self.cs.set_low().ok();
                buf[0] = REG_IRQ_FLAGS | 0x80;
                self.spi.transfer(&mut receive_buf, &mut buf).ok();
                if buf[1] & IRQ_RX_DONE_MASK != 0 {
                    //self.cs.set_high().ok();
                    let mut len = [0];
                    self.read_reg(REG_RX_NB_BYTES, &mut len).ok();
                    data.reserve(len[0] as usize);
                    self.read_reg(REG_FIFO, &mut data).ok();
                    done = true;
                } else {
                    //self.cs.set_high().ok();
                }
            }
            t += 1;
            FreeRtos::delay_us(1000);
        }
        if t >= timeout {
            return Err(());
        }
        Ok(data)
    }

    fn read_reg(&mut self, reg: u8, data: &mut [u8]) -> Result<(), EspError> {
        let mut buf = [0; 2];
        //self.cs.set_low().ok();
        let mut receive_buf: [u8; 1]= [0];
        buf[0] = reg & 0x7F;
        self.spi.transfer(&mut receive_buf, &mut buf)?;
        //self.cs.set_high().ok();
        //self.cs.set_low().ok();
        buf[0] = 0x00;
        for d in data {
            buf[1] = 0;
            self.spi.transfer(&mut receive_buf, &mut buf)?;
            *d = buf[1];
        }
        //self.cs.set_high().ok();
        Ok(())
    }

    fn write_reg(&mut self, reg: u8, data: &[u8]) -> Result<(), EspError> {
        let mut buf = [0; 2];
        //self.cs.set_low().ok();
        let mut receive_buf: [u8; 1]= [0];
        buf[0] = reg | 0x80;
        for d in data {
            buf[1] = *d;
            self.spi.transfer(&mut receive_buf, &mut buf).ok();
        }
        //self.cs.set_high().ok();
        Ok(())
    }
}
    
    