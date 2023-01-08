use esp_idf_hal::{uart::UartDriver, delay::BLOCK};
use esp_idf_sys::EspError;





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

    pub fn init_gps() -> Result<(), EspError> {
        uart.write(b"$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n");
    }

    pub fn get_data(&mut self) -> GPSData {
        let mut data = String::new();

        // Read data from the GPS device until we receive a newline character
        while !data.ends_with('\n') {
            let mut buf = [0; 128];
            let n = self.serial.read(&mut buf).unwrap();
            data.push_str(&String::from_utf8_lossy(&buf[..n]));
        }

        // Parse the GPS data
        let fields: Vec<&str> = data.split(',').collect();
        if fields[0] != "$GPRMC" {
            // Not a valid GPRMC sentence, return an error
            return GPSData { /* error */ };
        }

        // Parse the rest of the fields
        let time = fields[1];
        let status = fields[2];
        let latitude = fields[3];
        let latitude_hemisphere = fields[4];
        let longitude = fields[5];
        let longitude_hemisphere = fields[6];
        let speed = fields[7];
        let track_angle = fields[8];
        let date = fields[9];
        let magnetic_variation = fields[10];
        let magnetic_variation_direction = fields[11];
        let mode = fields[12];

        GPSData {
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
        }
    }

}