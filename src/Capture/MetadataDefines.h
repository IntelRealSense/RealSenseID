namespace RealSenseID
{
namespace Capture
{
typedef struct
{
    uint32_t md_type_id; // The type of the metadata struct. Shall be assigned the value of METADATA_CAPTURE_ID
    uint32_t md_size;    // Actual size of metadata struct in bytes not including header, 71 bytes in this version
} md_header_t;

struct uvc_header
{
    uint8_t length;
    uint8_t info;
    uint32_t timestamp;
    uint8_t source_clock[6];
};

typedef struct
{
    md_header_t header;
    uint32_t version;          // shall have the value of MD_CAPTURE_INFO_VER
    uint32_t flags;            // Bit array to specify attributes that are valid.
    uint32_t frame_counter;    // Sequential frame number
    uint32_t sensor_timestamp; // Microsecond unit
    uint32_t exposure_time;    // The exposure time in microsecond unit
    uint16_t gain_value;       // Sensor's gain (UVC standard)
    uint8_t led_status;        // LED On/Off
    uint8_t laser_status;      // Projector On/Off
    uint8_t preset_id;         // FA selected preset type (enumerated)
    /// Added attributes (using 38 bytes reserved)
    uint8_t sensor_id;       // sensor id (right = 0 , left =1)
    uint8_t status;          // enroll status
    uint8_t valid_face_rect; // indicates valid faceRect
    uint8_t reserved[24];    // bytes reserved for future modifications.
} md_middle_level;

constexpr uint8_t uvc_header_size = sizeof(md_header_t);
constexpr uint8_t md_middle_level_size = sizeof(md_middle_level);

} // namespace Capture
} // namespace RealSenseID