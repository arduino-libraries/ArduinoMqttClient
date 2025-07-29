#pragma once
#include "../MqttInterface.h"
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>

// TODO calculate minimum size
#define CONFIG_MQTT_CLIENT_RX_BUFFER 128
#define CONFIG_MQTT_CLIENT_TX_BUFFER 128

class ZephyrMqttClient: public MqttClientInterface {
public:
    ZephyrMqttClient();
    ~ZephyrMqttClient();

    int connect(IPAddress ip, uint16_t port) override;
    int connect(const char *host, uint16_t port) override;
    void disconnect() override;

    uint8_t connected();
    operator bool();

    error_t subscribe(
        Topic t,
        MqttQos qos = QosDefault) override;

    error_t publish(
        Topic t, uint8_t payload[],
        size_t size, MqttQos qos = QosDefault,
        MqttPublishFlag flags = MqttPublishFlags::None) override;

    error_t unsubscribe(Topic t) override;
    void poll() override;
    error_t ping() override;

    void setReceiveCallback(MqttReceiveCallback cbk) override;
    void setId(const char* client_id = nullptr) override;
    void setUsernamePassword(const char* username, const char* password=nullptr) override;
    void setWill(
        Topic willTopic, const uint8_t* will_message,
        size_t will_size, MqttQos qos=QosDefault,
        MqttPublishFlag flags = MqttPublishFlags::None) override;

    // TODO should this be private?
    void mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

    // in zephyr we do not need to provide a client on which to rely on
    void setClient(arduino::Client*) override {};

    int read() override;
    int read(uint8_t payload[], size_t len) override;
    int available() override;

    String messageTopic() const override;
    int messageDup()      const override;
    uint16_t messageId()  const override;
    MqttQos messageQoS()  const override;
    int messageRetain()   const override;
private:
    int connect(sockaddr_storage addr);
    void client_init();

    struct mqtt_client client;
    struct zsock_pollfd fds;
    sockaddr_storage broker; // broker address TODO should we keep it as an attribute

    uint8_t rx_buffer[CONFIG_MQTT_CLIENT_RX_BUFFER];
    uint8_t tx_buffer[CONFIG_MQTT_CLIENT_TX_BUFFER];

    MqttReceiveCallback _cbk;

    struct mqtt_publish_param *last_message;
};
