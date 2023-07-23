#[cfg(test)]
mod header_tests {
    use crate::network::packet;
    use crate::network::packet::PacketHeader;

    #[test]
    #[should_panic]
    fn test_lora_packet_header() {
        let header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0xFF,
            0xFF,
            0xFFFF
        ).unwrap();

    }

    #[test]
    fn test_lora_packet_header_crc_calculation() {
        let mut header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        header.calculate_header_crc();

        assert_eq!(header.header_crc, 0xED9Bu16);
    }

    #[test]
    fn test_lora_packet_header_crc_checker() {
        let mut header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        header.calculate_header_crc();

        assert_eq!(header.check_header_crc(), true);
    }

    #[test]
    fn test_lora_packet_header_into_vec_u8() {
        let header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        let header_vec: Vec<u8> = header.into();

        let assert_header: Vec<u8> = vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF];

        assert_eq!(header_vec, assert_header);
    }

    #[test]
    #[should_panic]
    fn test_lora_packet_header_tryfrom_fail(){
        packet::LoRaPacketHeader::try_from(vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF, 0x44, 0x33, 0x44]).unwrap();

    }

    #[test]
    fn test_lora_packet_header_tryfrom() {
        let header = packet::LoRaPacketHeader::try_from(vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xBF, 0xFF]).unwrap();

        let assert_header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xBFFF
        ).unwrap();

        assert_eq!(header, assert_header);
    }

    #[test]
    fn test_lora_packet_header_into_array_u8() {
        let header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xBFFF
        ).unwrap();

        let header_array: [u8; 7] = header.into();

        let assert_header: [u8; 7] = [0x01, 0x02, 0x03, 0x04, 0x05, 0xBF, 0xFF];

        assert_eq!(header_array, assert_header);
    }

}

#[cfg(test)]
mod payload_tests {
    use crate::network::packet::LoRaPacketPayload;

    #[test]
    #[should_panic]
    fn packet_new_should_fail() {
        let payload = LoRaPacketPayload::new(0xFFFF, (0..255).collect::<Vec<u8>>()).unwrap();
    }

    #[test]
    fn packet_new() {
        let payload = LoRaPacketPayload::new(0xFFFF, (0..6).collect::<Vec<u8>>()).unwrap();

        assert_eq!(payload.payload, vec![0,1, 2, 3, 4, 5]);

    }

    #[test]
    #[should_panic]
    fn payload_tryfrom_vec_u8_should_fail() {
        let payload = LoRaPacketPayload::new(0xFFFF, (0..255).collect::<Vec<u8>>()).unwrap();
    }

    #[test]
    fn payload_tryfrom_vec_u8() {
        let payload = LoRaPacketPayload::try_from((0..6).collect::<Vec<u8>>()).unwrap();

        assert_eq!(payload.payload, vec![2, 3, 4, 5]);
    }

    #[test]
    fn payload_into_vec_u8() {
        let payload = LoRaPacketPayload::try_from((0..6).collect::<Vec<u8>>()).unwrap();

        let payload_vec: Vec<u8> = payload.into();

        assert_eq!(payload_vec, vec![0, 1, 2, 3, 4, 5]);
    }
}


#[cfg(test)]
mod packet_test {
    use crate::network::packet::LoRaPacket;



}