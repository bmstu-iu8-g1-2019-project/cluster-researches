// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <config.hpp>
#include <behavior.hpp>

class MemberMatrix {
private:
    std::unordered_map<MemberAddr, size_t ,MemberAddr::Hasher> _indexes;
    std::vector<std::vector<Member>> _matrix;

    std::vector<TimeStamp> _fd;

    Member _nullMember;

public:
    MemberMatrix()
      : _nullMember{MemberAddr{ip_v4::from_string("0.0.0.0"), 0}}
    {
        _nullMember.Info().Status() = MemberInfo::Dead;
    }

    void Push(PullGossip&& pull) {
        if (!_IsInMatrix(pull.Owner())) {
            _NewRow(pull.Owner());
        }
        auto ownerFound = _indexes.find(pull.Owner().Addr());

        auto table = pull.MoveTable();
        for (size_t i = 0; i < table.Size(); ++i) {
            auto neighbour = table[i];

            if (!_IsInMatrix(neighbour)) {
                _NewRow(neighbour);
            }
            auto neighbourFound = _indexes.find(neighbour.Addr());

            _matrix[ownerFound->second][neighbourFound->second] = std::move(neighbour);
        }
        _matrix[ownerFound->second][ownerFound->second] = pull.Owner();
        _fd[ownerFound->second] = TimeStamp::Now();
    }

    void DetectFailure(milliseconds timeout) {
        for (size_t i = 0; i < _fd.size(); ++i) {
            if (_fd[i].Time() != milliseconds{0} &&
                _fd[i].TimeDistance(TimeStamp::Now()) > timeout) {
                _fd[i] = TimeStamp{milliseconds{0}};

                for (auto& member : _matrix[i]) {
                    member.Info().Status() = MemberInfo::Dead;
                }
            }
        }
    }

    void Log(std::ostream& out) const {
        system("clear");
        for (auto i = 0; i < _matrix.size(); ++i) {
            for (auto j = 0; j < _matrix.front().size(); ++j) {
                switch (_matrix[i][j].Info().Status()) {
                    case MemberInfo::Alive :
                        out << "\033[0;32mA\033[0m";
                        break;
                    case MemberInfo::Suspicious :
                        out << "\033[0;33mS\033[0m";
                        break;
                    case MemberInfo::Dead :
                        out << "\033[1;31mF\033[0m";
                }
                out << " ";
            }
            out << " Member " << i << " : " << _matrix[i][i].ToJSON()
                << std::endl;
        }
        out << "Cluster size : " << _matrix.size()
            << std::endl;
    }



private:
    bool _IsInMatrix(const Member& member) const {
        return _indexes.find(member.Addr()) != _indexes.end();
    }

    void _NewRow(const Member& member) {
        _indexes.emplace(std::make_pair(member.Addr(), _matrix.size()));
        _matrix.emplace_back(std::vector<Member>(_matrix.size(), _nullMember));
        _fd.emplace_back(TimeStamp::Now());

        for (auto& row : _matrix) {
            row.push_back(_nullMember);
        }
    }
};


int main(int argc, char** argv) {
    Config config{argv[1], argv[2], argv[3]};

    boost::asio::io_service ioService;
    port_t port = (config.Containerization()) ? config.DockerPort() : config.Port();
    Socket socket{ioService, port, config.BufferSize()};

    ThreadSaveQueue<PullGossip> pullQ;
    socket.RunObserving(pullQ);

    MemberMatrix matrix;
    while (true) {
        auto pulls = pullQ.Pop(config.ObserveNum());

        for (auto& pull : pulls) {
            matrix.Push(std::move(pull));
        }

        static TimeStamp logRepetition = TimeStamp::Now();
        if (logRepetition.TimeDistance(TimeStamp::Now()) > config.LogRepetition()) {
            matrix.Log(std::cout);

            logRepetition = TimeStamp::Now();
        }

        matrix.DetectFailure(config.ObserverFDTimout());


    }
}
