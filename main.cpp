#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "dtl/dtl.hpp"
#include <sstream>


using dtl::Diff;
using dtl::elemInfo;
using dtl::uniHunk;

template<typename T>
T s2f(const std::string &s) {
    std::stringstream i;
    i << s;
    T r;
    i >> r;
    return r;
}

class log_record{
    public:
        std::string function_address;
        std::string function_latency;

    log_record() {
    }

    void set_address(std::string address) {
        function_address = address;
    }

    void set_latency(std::string latency) {
        function_latency = latency;
    }

    bool isEqual(const log_record& rhs) const {
        return function_address == rhs.function_address;
    }

    bool isOutLayer() const {
        return s2f<double>(function_latency) < 1000;
    }

    friend bool operator==(const log_record& lhs, const log_record& rhs) {
        return lhs.isEqual(rhs);
    }


    friend std::ostream &operator << (std::ostream &o, const log_record &t) {
        return o << t.function_address << " " << t.function_latency << "ms";
    }

};

static void showStats (std::string fp1, std::string fp2);
static void unifiedDiff (std::vector<log_record> sequence1, std::vector<log_record> sequence2,std::ofstream& parsed_log);
void countCost(std::ifstream& parsed_log);

int main() {
    std::string line;
    char* log_path = "../../../projects/mysqld/s2e-out-4/debug.txt";
    std::string expression = "InstructionTracker: Function";
    std::string filer = "0xfffffffff";
    std::ifstream s2e_log(log_path);


    if (s2e_log.is_open()) {
        std::vector<log_record> sequence1;
        std::vector<log_record> sequence2;
        std::string token;
        std::ofstream parsed_log;
        parsed_log.open("result-compare.log");
        while (s2e_log.good()){
            getline (s2e_log,line);
            if (line.find(expression)!= std::string::npos && line.find(filer) == std::string::npos) {
                log_record record;
                std::stringstream stream(line);
                int cnt = 0;
                while (getline(stream, token, ' ')) {
                    if (cnt == 5) {
                        record.set_address(token);
                    } else if (cnt == 7) {
                        token.pop_back();
                        token.pop_back();
                        record.set_latency(token);
                    }
                    cnt++;
                }

                if (line.find("State 0")!= std::string::npos) {
                    sequence1.push_back(record);
                }

                if (line.find("State 1")!= std::string::npos)  {
                    sequence2.push_back(record);
                }
            }
        }
        unifiedDiff(sequence1,sequence2,parsed_log);
        parsed_log.close();

        std::ifstream count_log("result-compare.log");
        countCost(count_log);
        s2e_log.close();
    } else {
        std::cout << "Unable to open file";
    }

    return 0;
}

/*
* Count the cost
*/
void countCost(std::ifstream& count_log) {
    int inc_cost = 0;
    int des_cost = 0;
    std::string line;
    while (count_log.good()) {
        getline(count_log, line);
        std::string token;
        std::stringstream stream(line);
        int cnt = 0;
        while (getline(stream, token, ' ')) {
            if (cnt == 1 && line.front() == '+')
                inc_cost +=s2f<double>(token);
            if (cnt == 1 && line.front() == '-')
                des_cost +=s2f<double>(token);
            cnt++;
        }
    }

    std::cout << "when autocommit = 1, the cost is " <<inc_cost << "ms\n";
    std::cout << "when autocommit = 0, the cost is " <<des_cost << "ms\n";

}

static void unifiedDiff (std::vector<log_record> sequence1, std::vector<log_record> sequence2, std::ofstream& parsed_log) {
    dtl::Diff< log_record, std::vector<log_record> > diff(sequence1,sequence2);
    diff.onHuge();
    diff.compose();

    diff.composeUnifiedHunks();
    diff.printUnifiedFormat(parsed_log);

}