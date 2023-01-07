use esp_idf_hal::adc::{self, Atten11dB, AdcChannelDriver, Adc, ADC2, Attenuation};
use esp_idf_hal::adc::config::Config;
use esp_idf_hal::gpio::Gpio27;
use esp_idf_sys::EspError;

use crate::common::PeripherialState;

const COMPENSATED_ADC_HIGHEST_VALUE: u16 = 3130 - 128;


// Lowes value of adc is 128 according to the measurements
const ADC_VALUE_DIFFERENCE: u8 = 128;

pub struct Throttle<'a, T> where T: Adc, Atten11dB<T>: Attenuation<ADC2>{
    pub adc_value: u16,
    adc_driver: adc::AdcDriver<'a, adc::ADC2>,
    adc_pin: esp_idf_hal::adc::AdcChannelDriver<'a, Gpio27, Atten11dB<T>>
}

impl <'a, T> Throttle<'a, T> where T: Adc,Atten11dB<T>: Attenuation<ADC2>{
    pub fn new(mut adc_driver: adc::AdcDriver<'a, adc::ADC2>, mut adc_pin: esp_idf_hal::adc::AdcChannelDriver<'a, Gpio27, Atten11dB<T>>) -> Throttle<'a, T> {
        Throttle{ adc_value: 128 , adc_driver: adc_driver, adc_pin: adc_pin}
    }

    pub fn read_adc(&mut self) -> Result<u16, EspError> {
        self.adc_value = self.adc_driver.read(&mut self.adc_pin)?;
        Ok(self.adc_value.clone())
    }

    /// Neccessary conversion to calculate the percentage.
    pub fn compensate_adc_value(&mut self) -> Result<u16, EspError> {
        if self.read_adc()? < 128 {
            Ok(0)
        } else {
            Ok(self.read_adc()? - 128)
        }
    }

    pub fn as_percentage(&mut self) -> Result<u8, EspError> {
        let adc_percentage = (f32::from(self.adc_value - ADC_VALUE_DIFFERENCE as u16) / f32::from(COMPENSATED_ADC_HIGHEST_VALUE - ADC_VALUE_DIFFERENCE as u16) * 100 as f32) as u8;
        
        if adc_percentage >= 100 {
            Ok(100)
        } else {
            Ok(adc_percentage)
        }
    }
}


impl <'a, T> PeripherialState for Throttle<'a, T> where T: Adc, Atten11dB<T>: Attenuation<ADC2> {
    fn update_state(&mut self) {
        self.read_adc();
    }
}
