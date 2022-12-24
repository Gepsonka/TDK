

const ALTIMETER_ADDRESS: u8 = 0x58;

pub struct Altimeter {
    temperature: i8,
    pressure: u16
}


impl Altimeter {
    pub fn new() -> Altimeter {
        Altimeter{temperature: 0, pressure: 0}
    }

    pub fn get_id_of_sensor(i2c: &mut I2cDriver) -> u8 {
        let id: [u8; 1] = [0];
        i2c.read()


    }
}
