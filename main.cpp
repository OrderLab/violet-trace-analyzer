#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "dtl/dtl.hpp"
#include <sstream>
#include <assert.h>


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

class function_trace{
    public:
        std::string function;
        std::string execution_time;
        std::string caller;

    function_trace() {
    }

    void set_address(std::string address) {
        function = address;
    }

    void set_latency(std::string latency) {
        execution_time = latency;
    }

    bool isEqual(const function_trace& rhs) const {
        return function == rhs.function;
    }

    bool isOutLayer() const {
        return s2f<double>(execution_time) < 5000;
    }

    friend bool operator==(const function_trace& lhs, const function_trace& rhs) {
        return lhs.isEqual(rhs);
    }

    friend std::ostream &operator << (std::ostream &o, const function_trace &t) {
        return o << t.function << " " << t.execution_time << "ms";
    }

};

struct stateRecord {
    int id;
    int instruction_count;
    int syscall_count;
    double execution_time;
    std::vector<function_trace> trace;
};

static void showStats (std::string fp1, std::string fp2);
static void unifiedDiff (std::vector<function_trace> sequence1, std::vector<function_trace> sequence2,std::ofstream& parsed_log);
void countCost(std::ifstream& parsed_log);
bool is_caseResult(std::string line);
size_t getPosition(std::string filter,const std::string *line);
int get_stateId(const std::string *line);
std::string get_count(const std::string *line,std::string name);
std::string get_address(const std::string *line,std::string name);
std::string get_execution_time(const std::string *line,std::string name);
std::string get_count_base(const std::string *line,std::string name, char separator);

int main(int argc, char** argv) {
    std::string line;
    char* log_path;
    std::string expression = "LatencyTracker: Function";
    std::map<int, stateRecord> cost_table;

    if (argc != 2) {
        printf("The argument doesn't match\n");
        return -1;
    }

    log_path = argv[1];
    std::ifstream s2e_log(log_path);

    if (s2e_log.is_open()) {
        while (s2e_log.good()){
            int id;
            getline (s2e_log,line);
            if (is_caseResult(line)) {
                id = get_stateId(&line);
                int instructions = stoi(get_count(&line, "instruction"));
                int syscalls = stoi(get_count(&line, "syscall"));
                if (!cost_table.count(id)) {
                    struct stateRecord record;
                    record.syscall_count = syscalls;
                    record.instruction_count = instructions;
                    record.id = id;
                    record.execution_time = 0;
                    cost_table[id] = record;
                } else {
                    assert(cost_table.count(id) == 1);
                    struct stateRecord& record = cost_table[id];
                    assert(record.syscall_count == 0);
                    assert(record.instruction_count == 0);
                    record.syscall_count = syscalls;
                    record.instruction_count = instructions;
                }
            }

            if (line.find(expression)!= std::string::npos) {
                id = get_stateId(&line);
                function_trace function_trace;
                std::string function = get_address(&line, "Function");
                std::string parent = get_address(&line, "caller:");
                std::string execution = get_execution_time(&line, "runs");
                function_trace.function = function;
                function_trace.caller = parent;
                function_trace.execution_time = execution;
                if (!cost_table.count(id)) {
                    struct stateRecord record;
                    record.syscall_count = 0;
                    record.instruction_count = 0;
                    record.id = id;

                    if (function_trace.caller == "0x0") {
                        record.execution_time = s2f<double>(function_trace.execution_time);
                    } else {
                        record.execution_time = 0;
                    }

                    record.trace.push_back(function_trace);
                    cost_table[id] = record;
                } else {
                    assert(cost_table.count(id) == 1);
                    struct stateRecord& record = cost_table[id];
                    record.syscall_count = 0;
                    record.instruction_count = 0;
                    record.id = id;
                    if (function_trace.caller == "0x0") {
                        record.execution_time += s2f<double>(function_trace.execution_time);
                    }
                    record.trace.push_back(function_trace);

                }
            }
        }
    } else {
        std::cout << "Unable to open file at " << log_path << "\n";
    }
    std::ofstream parsed_log;
    parsed_log.open("result-compare.log");
    for (auto record_iterator=cost_table.begin(); record_iterator!=cost_table.end(); ++record_iterator) {
        parsed_log << record_iterator->first << " => the instruction is " << record_iterator->second.instruction_count << ",the syscall is " << record_iterator->second.syscall_count
                   << ", the execution time " << record_iterator->second.execution_time <<"ms\n";
        for (std::vector<function_trace>::iterator function_trace = record_iterator->second.trace.begin();function_trace != record_iterator->second.trace.end();function_trace++){
            if (s2f<double>(function_trace->execution_time) >100) {
                parsed_log << "Function " << function_trace->function << ", caller " << function_trace->caller << ", execution time " << function_trace->execution_time <<"\n";
            }
        }
    }
        

//    if (s2e_log.is_open()) {
//        std::vector<function_trace> sequence1;
//        std::vector<function_trace> sequence2;
//        std::string token;
//        std::ofstream parsed_log;
//        parsed_log.open("result-compare.log");
//        while (s2e_log.good()){
//            getline (s2e_log,line);
//            if (line.find(expression)!= std::string::npos && line.find(filter) == std::string::npos) {
//                function_trace record;
//                std::stringstream stream(line);
//                int cnt = 0;
//                while (getline(stream, token, ' ')) {
//                    if (cnt == 5) {
//                        record.set_address(token);
//                    } else if (cnt == 7) {
//                        token.pop_back();
//                        token.pop_back();
//                        record.set_latency(token);
//                    }
//                    cnt++;
//                }
//
//                if (line.find("State 0")!= std::string::npos) {
//                    sequence1.push_back(record);
//                }
//
//                if (line.find("State 1")!= std::string::npos)  {
//                    sequence2.push_back(record);
//                }
//            }
//        }
//        unifiedDiff(sequence1,sequence2,parsed_log);
//        parsed_log.close();
//
//        std::ifstream count_log("result-compare.log");
//        countCost(count_log);
//        s2e_log.close();
//    } else {
//        std::cout << "Unable to open file";
//    }

    return 0;
}

std::string get_execution_time(const std::string *line,std::string name){
    return get_count_base(line,name,';');
}

std::string get_address(const std::string *line,std::string name){
    return get_count_base(line,name,';');
}

std::string get_count(const std::string *line,std::string name){
    return get_count_base(line,name,';');
}

std::string get_count_base(const std::string *line,std::string name, char separator){
    size_t position;
    std::string token;
    position = getPosition(name ,line) + name.length() + 1;
    std::stringstream stream(line->substr(position));
    getline(stream, token, separator);
    return token;
}

int get_stateId(const std::string *line) {
    size_t position;
    std::string token;
    std::string probe = "State";
    position = getPosition(probe ,line) + probe.length() + 1;
    std::stringstream stream(line->substr(position));
    getline(stream, token, ']');
    return stoi(token);
}

bool is_caseResult(std::string line) {
    std::string expression = "TestCaseGenerator: generating test case at address";
    if (line.find(expression)!= std::string::npos)
        return true;
    else
        return false;
}

size_t getPosition(std::string filter,const std::string *line) {
    size_t found = line->find(filter);
    if (found == std::string::npos)
        std::cout << "Can not find <" << filter << "> in line <" << line << ">\n";
    return found;

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

static void unifiedDiff (std::vector<function_trace> sequence1, std::vector<function_trace> sequence2, std::ofstream& parsed_log) {
    dtl::Diff< function_trace, std::vector<function_trace> > diff(sequence1,sequence2);
    diff.onHuge();
    diff.compose();

    diff.composeUnifiedHunks();
    diff.printUnifiedFormat(parsed_log);

}