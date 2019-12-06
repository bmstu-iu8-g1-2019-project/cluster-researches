// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <config.hpp>


Config::Config(char* configPath, char* strIP, char* strPort) {
    std::cout << configPath << std::endl;
    std::cout << strIP << std::endl;

    std::ifstream file{configPath};

    if (!file.is_open()) {
        throw std::runtime_error{"Unable to open config file"};
    }

    auto json = nlohmann::json::parse(file);

    auto found = json.find("containerization");
    if (found != json.end()) {
        _containerization = json["containerization"];
    } else {
        _containerization = false;
        std::cout << "Set default \"containerization\": "
                  << std::boolalpha << _containerization << std::endl;
    }

    if (!_containerization) {
        _dockerPort = 0;
    } else {
        _dockerPort = json["docker-port"];
        std::cout << "Set docker port : " << _dockerPort << std::endl;
    }

    _ip = ip_v4::from_string(strIP);
    _port = std::stoul(strPort);
    std::cout << "Set port : " << _port << std::endl;


    found = json.find("buffer-size");
    if (found != json.end()) {
        _bufferSize = json["buffer-size"];
    } else {
        _bufferSize = 1500;
        std::cout << "Set default \"buffer-size\": " << _bufferSize << std::endl;
    }


    found = json.find("latest-part-size");
    if (found != json.end()) {
        _latestPartSize = json["latest-part-size"];
    } else {
        _latestPartSize = 10;
        std::cout << "Set default \"latest-part-size\": " << _latestPartSize << std::endl;
    }

    found = json.find("random-part-size");
    if (found != json.end()) {
        _randomPartSize = json["random-part-size"];
    } else {
        _randomPartSize = 10;
        std::cout << "Set default \"random-part-size\": " << _randomPartSize << std::endl;
    }


    found = json.find("ping-processing-num");
    if (found != json.end()) {
        _pingProcNum = json["ping-processing-num"];
    } else {
        _pingProcNum = 10;
        std::cout << "Set default \"ping-processing-num\": " << _pingProcNum << std::endl;
    }

    found = json.find("ack-processing-num");
    if (found != json.end()) {
        _ackProcNum = json["ack-processing-num"];
    } else {
        _ackProcNum = 10;
        std::cout << "Set default \"ack-processing-num\": " << _ackProcNum << std::endl;
    }


    found = json.find("spread-destination-num");
    if (found != json.end()) {
        _spreadDestsNum = json["spread-destination-num"];
    } else {
        _spreadDestsNum = 4;
        std::cout << "Set default \"spread-destination-num\": " << _spreadDestsNum << std::endl;
    }

    found = json.find("spread-repetition");
    if (found != json.end()) {
        _spreadRepetition = milliseconds{json["spread-repetition"]};
    } else {
        _spreadRepetition = milliseconds{1000};
        std::cout << "Set default \"spread-repetition\": " << _spreadRepetition.count() << std::endl;
    }


    found = json.find("fd-timeout");
    if (found != json.end()) {
        _fdTimeout = milliseconds{json["fd-timeout"]};
    } else {
        _fdTimeout = milliseconds{500};
        std::cout << "Set default \"fd-timeout\": " << _fdTimeout.count() << std::endl;
    }

    found = json.find("fd-repetition");
    if (found != json.end()) {
        _fdRepetition = milliseconds{json["fd-repetition"]};
    } else {
        _fdRepetition = milliseconds{100};
        std::cout << "Set default \"fd-repetition\": " << _fdRepetition.count() << std::endl;
    }

    found = json.find("fd-failures-before-dead");
    if (found != json.end()) {
        _fdFailuresBeforeDead = json["fd-failures-before-dead"];
    } else {
        _fdFailuresBeforeDead = 1;
        std::cout << "Set default \"fd-failures-before-dead\": " << _fdFailuresBeforeDead << std::endl;
    }


    _observerIP = ip_v4::from_string(json["observer-ip"]);
    _observerPort = json["observer-port"];

    found = json.find("observe-repetition");
    if (found != json.end()) {
        _observeRepetition = milliseconds{json["observe-repetition"]};
    } else {
        _observeRepetition = milliseconds{200};
        std::cout << "Set default \"observe-repetition\": " << _observeRepetition.count() << std::endl;
    }

    found = json.find("observe-num");
    if (found != json.end()) {
        _observeNum = json["observe-num"];
    } else {
        _observeNum = 100;
        std::cout << "Set default \"observe-num\": " << _observeNum << std::endl;
    }

    found = json.find("observer-fd-timeout");
    if (found != json.end()) {
        _observerFDTimeout = milliseconds{json["observer-fd-timeout"]};
    } else {
        _observerFDTimeout = milliseconds{1000};
    }

    found = json.find("log-repetition");
    if (found != json.end()) {
        _logRepetition = milliseconds{json["log-repetition"]};
    } else {
        _logRepetition = milliseconds{1000};
        std::cout << "Set default \"log-repetition\": " << _logRepetition.count() << std::endl;
    }

    _pTable = std::make_unique<Table>(MemberAddr{_ip, _port}, _latestPartSize, _randomPartSize);
    for (const auto& jsonMember : json["entry-point"]) {
        auto member = Member{MemberAddr::FromJSON(jsonMember["address"])};
        _pTable->Update(member);
    }
}

bool Config::Containerization() const {
    return _containerization;
}

port_t Config::DockerPort() const {
    return _dockerPort;
}

const ip_v4& Config::IP() const {
    return _ip;
}

port_t Config::Port() const {
    return _port;
}

size_t Config::BufferSize() const {
    return _bufferSize;
}

size_t Config::LatestPartSize() const {
    return _latestPartSize;
}

size_t Config::RandomPartSize() const {
    return _randomPartSize;
}

size_t Config::PingProcessingNum() const {
    return _pingProcNum;
}

size_t Config::AckProcessingNum() const {
    return _ackProcNum;
}

size_t Config::SpreadDestinationNum() const {
    return _spreadDestsNum;
}

milliseconds Config::SpreadRepetition() const {
    return _spreadRepetition;
}

milliseconds Config::FDTimout() const {
    return _fdTimeout;
}

milliseconds Config::FDRepetition() const {
    return _fdRepetition;
}

size_t Config::FailuresBeforeDead() const {
    return _fdFailuresBeforeDead;
}

const ip_v4& Config::ObserverIP() const {
    return _observerIP;
}

port_t Config::ObserverPort() const {
    return _observerPort;
}

milliseconds Config::ObserveRepetition() const {
    return _observeRepetition;
}

size_t Config::ObserveNum() const {
    return _observeNum;
}

milliseconds Config::ObserverFDTimout() const {
    return _observerFDTimeout;
}

milliseconds Config::LogRepetition() const {
    return _logRepetition;
}

Table&& Config::MoveTable() {
    return std::move(*_pTable);
}
