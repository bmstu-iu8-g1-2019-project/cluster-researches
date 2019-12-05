// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

class MemberMatrix {
private:
    std::unordered_map<MemberAddr, size_t ,MemberAddr::Hasher> _indexes;
    std::vector<std::vector<Member>> _matrix;

    std::vector<TimeStamp> _fd;

public:
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
                _matrix[i][i].Info().Status() = MemberInfo::Dead;
                _fd[i] = TimeStamp{milliseconds{0}};
                std::cout << "Detected failure" << std::endl;
            }
        }
    }

    void Log(std::ostream& out) const {
        for (const auto& row : _matrix) {
            for (const auto& element : row) {
                switch (element.Info().Status()) {
                    case MemberInfo::Alive :
                        out << " ";
                        break;
                    case MemberInfo::Suspicious :
                        out << "s";
                        break;
                    case MemberInfo::Dead :
                        out << "D";
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
        _fd.emplace_back(TimeStamp::Now());

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

    ThreadSaveQueue<PullGossip> pullQ;
    socket.RunObserving(pullQ);

    MemberMatrix matrix;
    while (true) {
        // TODO(AndreevSemen) : do it env var
        auto pulls = pullQ.Pop(config.ObserveNum());

        for (auto& pull : pulls) {
            matrix.Push(std::move(pull));
        }

        static TimeStamp logRepetition = TimeStamp::Now();
        if (logRepetition.TimeDistance(TimeStamp::Now()) > config.LogRepetition()) {
            matrix.Log(std::cout);

            logRepetition = TimeStamp::Now();
        }

        matrix.DetectFailure(milliseconds{1500});
    }
}
