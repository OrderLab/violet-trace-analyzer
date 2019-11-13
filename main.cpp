#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "dtl/dtl.hpp"
#include "include/cxxopts.hpp"
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

class function_trace {
public:
    std::string function;
    std::string execution_time;
    double diff_execution;
    std::string caller;
    std::string activityId;
    std::string parentId;
    bool diff_flag;

    function_trace() {
        diff_execution = 0;
    }

    void set_address(std::string address) {
        function = address;
    }

    void set_latency(std::string latency) {
        execution_time = latency;
    }

    bool isEqual(const function_trace &rhs) const {
        return function == rhs.function;
    }

    bool isOutLayer() const {
        return s2f<double>(execution_time) < 5000;
    }

    friend bool operator==(const function_trace &lhs, const function_trace &rhs) {
        return lhs.isEqual(rhs);
    }

    friend std::ostream &operator<<(std::ostream &o, const function_trace &t) {
        return o << "; Function " << t.function << "; runs " << t.execution_time;
    }
};

struct stateRecord {
    int id;
    int instruction_count;
    int syscall_count;
    double execution_time;
    std::vector<function_trace> trace;
    std::vector<function_trace> diff_trace;
};

static void
unifiedDiff(std::vector<function_trace> original_trace, std::vector<function_trace> changed_trace,
            std::ofstream &parsed_log);

void countCost(std::ifstream &parsed_log);

bool is_caseResult(std::string line);

size_t getPosition(std::string filter, const std::string *line);

int get_stateId(const std::string *line);

std::string get_count(const std::string *line, std::string name);

std::string get_address(const std::string *line, std::string name);

std::string get_execution_time(const std::string *line, std::string name);

std::string get_count_base(const std::string *line, std::string name, char separator);

void create_critical_path(int state, stateRecord state_record, std::ofstream *parsed_log);

void diffTrace(std::map<int, stateRecord> *cost_table);

bool get_diff(const std::string *line);

void generateTestCases(std::map<int, stateRecord> *cost_table, std::string output_path);

int main(int argc, char **argv) {
    std::string line;
    std::string log_path;
    std::string output_path;
    std::string expression = "LatencyTracker: Function";
    std::map<int, stateRecord> cost_table;

    cxxopts::Options options("Log analyzer", "analyze the log of s2e result");
    options.add_options()
            ("p,path", "input file name", cxxopts::value<std::string>())
            ("o,output", "output file name",cxxopts::value<std::string>() )
            ;
    auto result = options.parse(argc, argv);
    log_path = result["path"].as<std::string>();
    output_path = result["output"].as<std::string>();

    std::ifstream s2e_log(log_path);

    if (s2e_log.is_open()) {
        while (s2e_log.good()) {
            int id;
            getline(s2e_log, line);
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
                    struct stateRecord &record = cost_table[id];
                    assert(record.syscall_count == 0);
                    assert(record.instruction_count == 0);
                    record.syscall_count = syscalls;
                    record.instruction_count = instructions;
                }
            }

            if (line.find(expression) != std::string::npos) {
                id = get_stateId(&line);
                function_trace function_trace;
                function_trace.activityId = get_address(&line, "activityId");
                function_trace.parentId = get_address(&line, "parentId");
                function_trace.function = get_address(&line, "Function");
                function_trace.caller = get_address(&line, "caller");
                function_trace.execution_time = get_execution_time(&line, "runs");
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
                    struct stateRecord &record = cost_table[id];
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

    s2e_log.close();
    generateTestCases(&cost_table,output_path);

    return 0;
}

void generateTestCases(std::map<int, stateRecord> *cost_table, std::string output_path) {
    std::ofstream parsed_log;
    parsed_log.open("temp.log");

    if (((*cost_table)[1].execution_time - (*cost_table)[0].execution_time) / (*cost_table)[0].execution_time > 0.2) {
        unifiedDiff((*cost_table)[0].trace, (*cost_table)[1].trace, parsed_log);
        parsed_log.close();

        diffTrace(cost_table);
        std::ofstream result;
        result.open(output_path);
        for (auto record_iterator = cost_table->begin(); record_iterator != cost_table->end(); ++record_iterator) {
            result << "[State " << record_iterator->first << "] => the number of instruction is "
                   << record_iterator->second.instruction_count << ",the number of syscall is "
                   << record_iterator->second.syscall_count
                   << ", the total execution time " << record_iterator->second.execution_time << "ms\n";
        }
        create_critical_path(1, (*cost_table)[1], &result);
        result.close();
    } else {
        std::ofstream result;
        result.open(output_path);
        for (auto record_iterator = cost_table->begin(); record_iterator != cost_table->end(); ++record_iterator) {
            result << "[State " << record_iterator->first << "] => the number of instruction is "
                   << record_iterator->second.instruction_count << ",the number of syscall is "
                   << record_iterator->second.syscall_count
                   << ", the total execution time " << record_iterator->second.execution_time << "ms\n";
        }
    }
}

void diffTrace(std::map<int, stateRecord> *cost_table) {
    std::ifstream diff_log("temp.log");
    if (diff_log.is_open()) {
        while (diff_log.good()) {
            bool symbol;
            function_trace function_trace;
            std::string line;
            getline(diff_log, line);
            if (line == "")
                continue;
            symbol = get_diff(&line);
            std::string function = get_address(&line, "Function");
            std::string execution = get_execution_time(&line, "runs");
            function_trace.function = function;
            function_trace.execution_time = execution;
            function_trace.diff_flag = symbol;
            (*cost_table)[1].diff_trace.push_back(function_trace);
        }
    }
    int count = 0;
    int diff_count = 0;
    int diff_size = (*cost_table)[1].diff_trace.size();
    int size = (*cost_table)[0].trace.size();
    for (auto record = (*cost_table)[1].trace.begin(); record != (*cost_table)[1].trace.end(); ++record) {
        if (count >= size) {
            count = size - 1;
        }

        if (diff_count >= diff_size) {
            diff_count = diff_size - 1;
        }

        function_trace original_trace = (*cost_table)[0].trace.at(count);
        function_trace diff_trace = (*cost_table)[1].diff_trace.at(diff_count);

        while (!diff_trace.diff_flag && (diff_trace.function == original_trace.function)) {
            count++;
            diff_count++;
            original_trace = (*cost_table)[0].trace.at(count);
            diff_trace = (*cost_table)[1].diff_trace.at(diff_count);
        }
        if (original_trace.function == record->function) {
            record->diff_execution = s2f<double>(record->execution_time) - s2f<double>(original_trace.execution_time);
            count++;
        } else if (diff_trace.diff_flag && (diff_trace.function == record->function)) {
            record->diff_execution = s2f<double>(record->execution_time);
            diff_count++;
        } else {
            printf("A error occurs\n");
        }
    }

}


bool get_diff(const std::string *line) {
    std::string token;
    std::stringstream stream(*line);
    getline(stream, token, ';');
    if (token == "+") {
        return true;
    } else if (token == "-") {
        return false;
    }
    assert(false); // should reach here
    return false;
}

//bool cmp()
//
//vector<function_trace*>x(N, NULL);
//sort(x.begin(), x.end(), cmp);
//res = unique(x.begin(), x.end(), cmp);
//x.resize(distance(x.begin(), res));
//lower_bound(x.begin(), x.end(), cmp, elem);
void create_critical_path(int state, stateRecord state_record, std::ofstream *parsed_log) {
    std::string parentId = "-1";
    for (int i = 0; i< 15; i++) {
        double m_diff = 0;
        function_trace result;
        int postion = 0;
        int count = 0;
        for (std::vector<function_trace>::iterator function_trace = state_record.trace.begin();
            function_trace != state_record.trace.end(); function_trace++) {
            count++;
            if (function_trace->parentId == parentId) {
                if (function_trace->diff_execution>m_diff && function_trace->function !="0x59a448" && function_trace->function !="0x3d98cb"){
                    m_diff = function_trace->diff_execution;
                    postion = count;
                }
            }
        }
        if (postion == 0)
            break;
        result = state_record.trace[postion-1];
        (*parsed_log)  << "[State " << state << "]" << " Function " << result.function << ", caller "
                   << result.caller
                   << ",activityId " << result.activityId << ",parentId " << result.parentId
                   << ", execution time " << result.execution_time << "; diff time "
                   << result.diff_execution << "ms\n";
        parentId = result.activityId;
    }


}

std::string get_execution_time(const std::string *line, std::string name) {
    return get_count_base(line, name, ';');
}

std::string get_address(const std::string *line, std::string name) {
    return get_count_base(line, name, ';');
}

std::string get_count(const std::string *line, std::string name) {
    return get_count_base(line, name, ';');
}

std::string get_count_base(const std::string *line, std::string name, char separator) {
    size_t position;
    std::string token;
    position = getPosition(name, line) + name.length() + 1;
    std::stringstream stream(line->substr(position));
    getline(stream, token, separator);
    return token;
}

int get_stateId(const std::string *line) {
    size_t position;
    std::string token;
    std::string probe = "State";
    position = getPosition(probe, line) + probe.length() + 1;
    std::stringstream stream(line->substr(position));
    getline(stream, token, ']');
    return stoi(token);
}

bool is_caseResult(std::string line) {
    std::string expression = "TestCaseGenerator: generating test case at address";
    if (line.find(expression) != std::string::npos)
        return true;
    else
        return false;
}

size_t getPosition(std::string filter, const std::string *line) {
    size_t found = line->find(filter);
    if (found == std::string::npos)
        std::cout << "Can not find <" << filter << "> in line <" << line << ">\n";
    return found;

}

/*
* Count the cost
*/
void countCost(std::ifstream &count_log) {
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
                inc_cost += s2f<double>(token);
            if (cnt == 1 && line.front() == '-')
                des_cost += s2f<double>(token);
            cnt++;
        }
    }

    std::cout << "when autocommit = 1, the cost is " << inc_cost << " ms\n";
    std::cout << "when autocommit = 0, the cost is " << des_cost << " ms\n";

}

static void
unifiedDiff(std::vector<function_trace> original_trace, std::vector<function_trace> changed_trace,
            std::ofstream &parsed_log) {
    dtl::Diff<function_trace, std::vector<function_trace> > diff(original_trace, changed_trace);
    diff.onHuge();
    diff.compose();

    diff.composeUnifiedHunks();
    diff.printUnifiedFormat(parsed_log);

}