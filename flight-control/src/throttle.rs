use esp_idf_hal::adc::{self, Atten11dB, AdcChannelDriver, Adc, ADC2, Attenuation};
use esp_idf_hal::adc::config::Config;
use esp_idf_hal::gpio::Gpio27;
use esp_idf_sys::EspError;

const ADC_LOWES_VALUE: u16 = 128;
const ADC_HIGHEST_VALUE: u16 = 3130;

const COMPENSATED_ADC_LOWES_VALUE: u16 = 0;
const COMPENSATED_ADC_Highest_VALUE: u16 = 3130 - 128;

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
    pub fn compensate_adc_value(&self) -> Result<u16, EspError> {
        if self.read_adc()? < 128 {
            Ok(0)
        } else {
            Ok(self.read_adc()? - 128)
        }
    }

    pub fn as_percentage(&self) -> Result<u16, EspError> {
        Ok(self.compensate_adc_value()? / COMPENSATED_ADC_Highest_VALUE)
    }
}