use esp_idf_hal::{uart::UartDriver, delay::BLOCK};
use esp_idf_sys::EspError;



pub struct GPSData {
    pub time: String,
    pub status: String,
    pub latitude: String,
    pub latitude_hemisphere: String,
    pub longitude: String,
    pub longitude_hemisphere: String,
    pub speed: String,
    pub track_angle: String,
    pub date: String,
    pub magnetic_variation: String,
    pub magnetic_variation_direction: String,
    pub mode: String,
}

pub struct GPS<'a> {
    uart: UartDriver<'a>,
    pub longitude: Option<f32>,
    pub latitude: Option<f32>,
    pub altitude: Option<f32>,
    pub number_of_satellites_in_view: u8
}


impl <'a> GPS<'a> {
    pub fn new(uart: UartDriver<'a>) -> Self {
        return GPS{ uart: uart, longitude: None, latitude: None, altitude: None, number_of_satellites_in_view: 0 }
    }

    pub fn init_gps(&self) -> Result<(), EspError> {
        self.uart.write(b"$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n")?;
        Ok(())
    }

    pub fn get_data(&mut self) -> Result<GPSData, ()> {
        let mut data = String::new();

        // Read data from the GPS device until we receive a newline character
        while !data.ends_with('\n') {
            let mut buf = [0; 128];
            let n = self.uart.read(&mut buf, BLOCK).unwrap();
            data.push_str(&String::from_utf8_lossy(&buf[..n]));
        }

        // Parse the GPS data
        let fields: Vec<&str> = data.split(',').collect();
        if fields[0] != "$GPRMC" {
            // Not a valid GPRMC sentence, return an error
            return Err(());
        }

        // Parse the rest of the fields
        let time = fields[1].to_string();
        let status = fields[2].to_string();
        let latitude = fields[3].to_string();
        let latitude_hemisphere = fields[4].to_string();
        let longitude = fields[5].to_string();
        let longitude_hemisphere = fields[6].to_string();
        let speed = fields[7].to_string();
        let track_angle = fields[8].to_string();
        let date = fields[9].to_string();
        let magnetic_variation = fields[10].to_string();
        let magnetic_variation_direction = fields[11].to_string();
        let mode = fields[12].to_string();

        Ok(GPSData {
            time,
            status,
            latitude,
            latitude_hemisphere,
            longitude,
            longitude_hemisphere,
            speed,
            track_angle,
            date,
            magnetic_variation,
            magnetic_variation_direction,
            mode,
        })
    }

    pub fn get_raw_data(&mut self) -> Result<String, EspError> {
        let mut buf = [0; 128];
        let n = self.uart.read(&mut buf, BLOCK).unwrap();
        return Ok(String::from_utf8(buf.to_vec()).unwrap());
    }

}