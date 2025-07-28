#pragma once
#include <functional>
#include <Client.h>

// The Idea for this section of the library is to allow the usage of different implementation for Mqtt Clients
// while preserving the possibility of having an Arduino standardized interface for Mqtt protocol
// One should implement MqttClientInterface and provide a way to instantiate the implementation

// namespace arduino { // namespace net { namespace mqtt {
typedef int error_t; // TODO move this to be generally available

class IStream {
public:
    virtual ~IStream() = default;

    virtual int available() = 0;
    virtual int read() = 0;
    // virtual int peek() = 0;
    virtual size_t readBytes(uint8_t *buffer, size_t length) = 0; // read chars from stream into buffer
};

class MqttOStream { // if OStream becomes available in arduino core api, we still need to extend its definition
public:
    MqttOStream(error_t rc=0): rc(rc) {}
    virtual ~MqttOStream() = default;

    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual int availableForWrite() = 0;

    const error_t rc;
};

using Topic = const char* const;

// for incoming published messages
using MqttReceiveCallback = std::function<void(Topic, IStream&)>;

// TODO MQTT 5.0 stuff

// TODO define callback for mqtt events. one should be the default, but the user can always change it

// copied from zephyr
// enum mqtt_conn_return_code: error_t {
// 	/** Connection accepted. */
// 	MQTT_CONNECTION_ACCEPTED                = 0x00,

// 	/** The Server does not support the level of the MQTT protocol
// 	 * requested by the Client.
// 	 */
// 	MQTT_UNACCEPTABLE_PROTOCOL_VERSION      = 0x01,

// 	/** The Client identifier is correct UTF-8 but not allowed by the
// 	 *  Server.
// 	 */
// 	MQTT_IDENTIFIER_REJECTED                = 0x02,

// 	/** The Network Connection has been made but the MQTT service is
// 	 *  unavailable.
// 	 */
// 	MQTT_SERVER_UNAVAILABLE                 = 0x03,

// 	/** The data in the user name or password is malformed. */
// 	MQTT_BAD_USER_NAME_OR_PASSWORD          = 0x04,

// 	/** The Client is not authorized to connect. */
// 	MQTT_NOT_AUTHORIZED                     = 0x05
// };

constexpr error_t NotImplementedError= -0x100; // TODO define a proper value

enum MqttQos: uint8_t {
    MqttQos0 = 0, // At Most once
    MqttQos1 = 1, // At least once
    MqttQos2 = 2, // Exactly once
};

typedef uint8_t MqttPublishFlag;
enum MqttPublishFlags: MqttPublishFlag {
    None            = 0,
    RetainEnabled   = 1,
    DupEnabled      = 2,
};

// TODO define mqtt version

constexpr MqttQos QosDefault = MqttQos0;
// constexpr size_t MqttClientIdMaxLength = 256;
constexpr size_t MqttClientIdMaxLength = 40;

// TODO it shouldn't be called client, since it is not an arduino client
class MqttClientInterface {
public:
    virtual ~MqttClientInterface() = default;

    virtual error_t connect(IPAddress ip, uint16_t port) = 0;
    virtual error_t connect(const char *host, uint16_t port) = 0; // TODO should host be string instead of c-string?
    virtual void disconnect() = 0;

    virtual error_t subscribe(Topic t, MqttQos qos = QosDefault) = 0;

    virtual error_t publish(
        Topic t, uint8_t payload[],
        size_t size, MqttQos qos = QosDefault,
        MqttPublishFlag flags = MqttPublishFlags::None) = 0;

    /**
     * Publish method for publishing messages in "streaming mode", meaning that
     * they can outsize the available ram. The OStream returned handles the lifecycle of
     * an mqtt Message, from the opening header to its termaination.
     * The concrete OStream is defined by the implementation
     */
    virtual MqttOStream&& publish(
        Topic t, MqttQos qos = QosDefault,
        MqttPublishFlag flags = MqttPublishFlags::None) = 0;

    virtual error_t unsubscribe(Topic t) = 0;
    virtual void poll() = 0;
    virtual error_t ping() = 0;

    virtual void setReceiveCallback(MqttReceiveCallback cbk) = 0;

    // nullptr means generate it randomly
    virtual void setId(const char* client_id = nullptr) = 0;

    // password may be null, if username is null password won't be used
    virtual void setUsernamePassword(const char* username, const char* password=nullptr) = 0;

    virtual void setWill(
        Topic willTopic, const uint8_t* will_message,
        size_t will_size, MqttQos qos=QosDefault,
        MqttPublishFlag flags = MqttPublishFlags::None) = 0;

    virtual void setClient(arduino::Client*) = 0;

    // FIXME the following definition may cause errors since one can easily pass a context dependent object
    // virtual void setClient(Client&) = 0;
    // Could this be a better solution?
    // virtual void setClient(Client&&) = 0;
};
