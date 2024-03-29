#include <iostream>
#include <sstream>

#include "session.hpp"


Session::Session(net::io_context& ioc, tcp::socket&& socket)
    : ioc_(ioc)
    , server_socket_(std::move(socket))
    , client_socket_(net::make_strand(ioc_))
{ }


void Session::Start()
{
    const tcp::endpoint target_endpoint = GetTargetEndpoint();
    Connect(target_endpoint);
}


tcp::endpoint Session::GetTargetEndpoint()
{
    // Смотрим адрес откуда подключение
    std::string source_ip = server_socket_.remote_endpoint().address().to_string();

    // Разбираем исходный адрес на отдельные октеты
    std::vector<std::string> octets;
    std::istringstream iss(source_ip);
    std::string octet;
    while (std::getline(iss, octet, '.')) {
        octets.push_back(octet);
    }

    if (octets.size() < 4) {
        std::cout << "Invalid IP address format\n";
    }

    // Извлекаем последнее число
    std::string last_octet = octets.back();

    // Преобразуем к unsigned int
    uint32_t last_octet_uint = std::stoi(last_octet);

    // Добавляем 2 (для простоты предполагаем что исходное число <= 253)
    last_octet_uint += 2;

    // Конверитируем новое число обратно в строку
    std::string new_last_octet = std::to_string(last_octet_uint);

    // Меняем последнее значение в векторе
    octets.back() = new_last_octet;

    // Собираем строку с новым ip
    std::string new_ip_addr = octets[0] + "." + octets[1] + "." + octets[2] + "." + octets[3];

    return tcp::endpoint(net::ip::make_address(new_ip_addr), TARGET_PORT);
}


void Session::Connect(const tcp::endpoint& target_endpoint)
{
    auto self(shared_from_this());

    client_socket_.async_connect(target_endpoint, [self](const sys::error_code& ec){
        if (!ec) {
            std::cout << "Connected to " << self->client_socket_.remote_endpoint().address().to_string() << std::endl;

            self->server_ = std::make_shared<SocketIOHandler>(std::move(self->server_socket_));
            self->client_ = std::make_shared<SocketIOHandler>(std::move(self->client_socket_));

            self->server_->SetOther(self->client_);
            self->client_->SetOther(self->server_);

            self->server_->Start();
            self->client_->Start();
        }
    }); 
}
