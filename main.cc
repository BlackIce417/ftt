#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

int create_data_connection(int ctrl_sockfd) {
    // 发送 PASV 命令
    std::string pasv_cmd = "PASV\r\n";
    send(ctrl_sockfd, pasv_cmd.c_str(), pasv_cmd.size(), 0);

    char buffer[1024] = {0};
    recv(ctrl_sockfd, buffer, sizeof(buffer), 0);
    std::cout << "PASV Response: " << buffer << std::endl;

    // 解析 PASV 响应
    int h1,h2,h3,h4,p1,p2;
    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1,&h2,&h3,&h4,&p1,&p2);
    std::string ip = std::to_string(h1)+"."+std::to_string(h2)+"."+std::to_string(h3)+"."+std::to_string(h4);
    int port = p1*256 + p2;

    // 创建数据 socket
    int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in data_addr{};
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &data_addr.sin_addr);

    if (connect(data_sockfd, (sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        std::cerr << "连接数据端口失败\n";
        return -1;
    }

    return data_sockfd;
}


int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(21);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "连接失败\n";
        return 1;
    }

    char buffer[1024];
    recv(sockfd, buffer, sizeof(buffer), 0);
    std::cout << buffer << std::endl;

    std::string user = "USER anonymous\r\n";
    send(sockfd, user.c_str(), user.size(), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    std::cout << "User response: " << buffer;

    std::string pass = "PASS 123456\r\n";
    send(sockfd, pass.c_str(), pass.size(), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    std::cout << "Password response: " << buffer;

    int data_sockfd = create_data_connection(sockfd);
    if (data_sockfd < 0) {
        std::cerr << "创建数据连接失败\n";
        close(sockfd);
        return 1;
    }

    std::ifstream file("./test.txt", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "文件打开失败\n";
        close(sockfd);
        close(data_sockfd);
        return 1;
    }

    std::string stor_cmd = "STOR /uploads/test_1.txt\r\n";
    send(sockfd, stor_cmd.c_str(), stor_cmd.size(), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    std::cout << "STOR response: " << buffer;

    std::cout << "开始上传文件...\n";

    char file_buffer[1024];
    while (!file.eof()) {
        file.read(file_buffer, sizeof(file_buffer));
        std::streamsize bytes = file.gcount();
        if (bytes > 0) {
            ssize_t send_bytes = send(data_sockfd, file_buffer, bytes, 0);
            if (send_bytes < 0) {
                std::cerr << "发送文件数据失败\n";
                break;
            }
            std::cout << "发送了 " << send_bytes << " 字节\n";
        }
    }
    shutdown(data_sockfd, SHUT_WR);

    std::cout << "文件上传完成\n";

    file.close();
    close(data_sockfd);
    close(sockfd);
    return 0;
}
