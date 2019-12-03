// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <observer.hpp>
#include <queue.hpp>
#include <behavior.hpp>

class MemberMatrix {
private:
    std::unordered_map<MemberAddr, size_t ,MemberAddr::Hasher> _indexes;
    std::vector<std::vector<Member>> _matrix;

public:
    void Push(const nlohmann::json& json) {
        Member owner = Member::FromJSON(json["owner"]);

        if (!_IsInMatrix(owner)) {
            _NewRow(owner);
        }
        auto ownerFound = _indexes.find(owner.Addr());

        for (const auto& jsonMember : json["neighbours"]) {
            auto neighbour = Member::FromJSON(jsonMember);

            if (!_IsInMatrix(neighbour)) {
                _NewRow(neighbour);
            }
            auto neighbourFound = _indexes.find(neighbour.Addr());

            _matrix[ownerFound->second][neighbourFound->second] = std::move(neighbour);
        }
        _matrix[ownerFound->second][ownerFound->second] = std::move(owner);
    }

    void Log(std::ostream& out) const {
        for (const auto& row : _matrix) {
            for (const auto& element : row) {
                switch (element.Info().Status()) {
                    case MemberInfo::Alive :
                        out << "   alive  ";
                        break;
                    case MemberInfo::Suspicious :
                        out << "suspicious";
                        break;
                    case MemberInfo::Dead :
                        out << "   DEAD   ";
                }
                out << "|";
            }
            out << std::endl;
        }
        out << std::endl;
    }

private:
    bool _IsInMatrix(const Member& member) const {
        return _indexes.find(member.Addr()) != _indexes.end();
    }

    void _NewRow(const Member& member) {
        static Member nullMember{MemberAddr{ip_v4::from_string("0.0.0.0"), 0}};

        _indexes.emplace(std::make_pair(member.Addr(), _matrix.size()));
        _matrix.emplace_back(std::vector<Member>(_matrix.size(), nullMember));

        for (auto& row : _matrix) {
            row.push_back(nullMember);
        }
    }
};


int main(int argc, char** argv) {
    Config config{argv[1], argv[2], argv[3]};

    boost::asio::io_service ioService;
    port_t port = (config.Containerization()) ? config.DockerPort() : config.Port();
    Socket socket{ioService, port, config.BufferSize()};

    ThreadSaveQueue<nlohmann::json> jsonQ;
    socket.RunJSONCatching(jsonQ);

    MemberMatrix matrix;
    while (true) {
        // TODO(AndreevSemen) : do it env var
        auto jsons = jsonQ.Pop(10);

        for (const auto& json : jsons) {
            matrix.Push(json);
        }

        static TimeStamp logRepetition = TimeStamp::Now();
        if (logRepetition.TimeDistance(TimeStamp::Now()) > config.LogRepetition()) {
            matrix.Log(std::cout);

            logRepetition = TimeStamp::Now();
        }
    }
}
