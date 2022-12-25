use esp_idf_hal::adc::{self, Atten11dB, AdcChannelDriver, Adc, ADC2, Attenuation};
use esp_idf_hal::adc::config::Config;
use esp_idf_hal::gpio::Gpio27;
use esp_idf_sys::EspError;

pub struct Throttle<'a, T> where T: Adc, Atten11dB<T>: Attenuation<ADC2>{
    pub adc_value: u16,
    adc_driver: adc::AdcDriver<'a, adc::ADC2>,
    adc_pin: esp_idf_hal::adc::AdcChannelDriver<'a, Gpio27, Atten11dB<T>>
}

impl <'a, T> Throttle<'a, T> where T: Adc,Atten11dB<T>: Attenuation<ADC2>{
    pub fn new(mut adc_driver: adc::AdcDriver<'a, adc::ADC2>, mut adc_pin: esp_idf_hal::adc::AdcChannelDriver<'a, Gpio27, Atten11dB<T>>) -> Throttle<'a, T> {
        Throttle{ adc_value: 128 , adc_driver: adc_driver, adc_pin: adc_pin}
    }

    pub fn read_adc(&mut self) -> Result<(), EspError> {
        self.adc_value = self.adc_driver.read(&mut self.adc_pin)?;
        Ok(())
    }
    // TODO: continue impl

    pub fn convert_to_percentage(&self) -> u8 {
        // TODO
        return 0;
    }
}