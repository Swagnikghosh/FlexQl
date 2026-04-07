#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "flexql.h"

namespace {

struct Capture {
    std::vector<std::vector<std::string>> rows;
};

int capture_row(void *arg, int column_count, char **values, char **) {
    auto *capture = static_cast<Capture *>(arg);
    std::vector<std::string> row;
    for (int i = 0; i < column_count; ++i) {
        row.emplace_back(values[i]);
    }
    capture->rows.push_back(std::move(row));
    return 0;
}

bool exec_ok(FlexQL *db, const std::string &sql, Capture *capture = nullptr) {
    char *errmsg = nullptr;
    int rc = flexql_exec(db, sql.c_str(), capture ? capture_row : nullptr, capture, &errmsg);
    if (rc != 0) {
        std::cerr << "query failed: " << sql << " error=" << (errmsg ? errmsg : "") << '\n';
        flexql_free(errmsg);
        return false;
    }
    flexql_free(errmsg);
    return true;
}

bool exec_batch_ok(FlexQL *db, const std::vector<std::string> &statements) {
    std::vector<const char *> statement_ptrs;
    statement_ptrs.reserve(statements.size());
    for (const auto &statement : statements) {
        statement_ptrs.push_back(statement.c_str());
    }

    char *errmsg = nullptr;
    int rc = flexql_exec_batch(db, statement_ptrs.data(), static_cast<int>(statement_ptrs.size()), &errmsg);
    if (rc != 0) {
        std::cerr << "batch failed:";
        for (const auto &statement : statements) {
            std::cerr << ' ' << statement;
        }
        std::cerr << " error=" << (errmsg ? errmsg : "") << '\n';
        flexql_free(errmsg);
        return false;
    }
    flexql_free(errmsg);
    return true;
}

bool exec_fails(FlexQL *db, const std::string &sql) {
    char *errmsg = nullptr;
    int rc = flexql_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (rc == 0) {
        std::cerr << "query unexpectedly succeeded: " << sql << '\n';
        flexql_free(errmsg);
        return false;
    }
    flexql_free(errmsg);
    return true;
}

bool contains_single_value(const Capture &capture, const std::string &value) {
    for (const auto &row : capture.rows) {
        if (!row.empty() && row[0] == value) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    std::system("rm -rf data/databases 2>/dev/null");
    std::thread server([]() { std::system("./bin/flexql-server 9100 >/tmp/flexql-server.log 2>&1"); });
    server.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    FlexQL *db = nullptr;
    if (flexql_open("127.0.0.1", 9100, &db) != 0) {
        std::cerr << "failed to open test connection\n";
        return 1;
    }

    if (!exec_ok(db, "cReAtE tAbLe STUDENT(ID INT PRIMARY KEY, NAME VARCHAR)")) {
        return 1;
    }
    if (!exec_ok(db, "create table if not exists student(ID INT PRIMARY KEY, NAME VARCHAR(255))")) {
        return 1;
    }
    if (!exec_fails(db, "create table student(ID INT PRIMARY KEY, NAME VARCHAR)")) {
        return 1;
    }
    if (!exec_ok(db, "create database SCHOOL")) {
        return 1;
    }
    if (!exec_ok(db, "create database if not exists school")) {
        return 1;
    }
    if (!exec_fails(db, "create database school")) {
        return 1;
    }
    if (!exec_ok(db, "use school")) {
        return 1;
    }
    if (!exec_ok(db, "create table Grades(STUDENT_ID INT, GRADE VARCHAR, PRIMARY KEY(STUDENT_ID))")) {
        return 1;
    }
    if (!exec_ok(db, "drop table if exists missing_table")) {
        return 1;
    }
    if (!exec_fails(db, "create table grades(STUDENT_ID INT, GRADE VARCHAR, PRIMARY KEY(STUDENT_ID))")) {
        return 1;
    }
    Capture db_capture;
    if (!exec_ok(db, "sHoW dAtAbAsEs", &db_capture)) {
        return 1;
    }
    if (!contains_single_value(db_capture, "school") || !contains_single_value(db_capture, "default")) {
        std::cerr << "SHOW DATABASES missing SCHOOL\n";
        return 1;
    }
    Capture table_capture;
    if (!exec_ok(db, "show tables from ScHoOl", &table_capture)) {
        return 1;
    }
    if (table_capture.rows.size() != 1 || table_capture.rows[0][0] != "grades") {
        std::cerr << "SHOW TABLES missing GRADES\n";
        return 1;
    }
    if (!exec_ok(db, "use default")) {
        return 1;
    }
    if (!exec_ok(db, "create table grades(STUDENT_ID INT, GRADE VARCHAR, PRIMARY KEY(STUDENT_ID))")) {
        return 1;
    }
    if (!exec_ok(db, "create table attendance(STUDENT_ID INT, STATUS VARCHAR, PRIMARY KEY(STUDENT_ID))")) {
        return 1;
    }
    if (!exec_ok(db, "insert into student values (1, 'Alice')")) {
        return 1;
    }
    if (!exec_ok(db, "InSeRt InTo StUdEnT VaLuEs (2, 'Bob')")) {
        return 1;
    }
    if (!exec_ok(db, "insert into GrAdEs values (1, 'A')")) {
        return 1;
    }
    if (!exec_ok(db, "insert into attendance values (1, 'Present')")) {
        return 1;
    }
    if (!exec_batch_ok(db, {"insert into student values (3, 'Cara')", "insert into student values (4, 'Eli')" })) {
        return 1;
    }
    if (!exec_fails(db, "insert into STUDENT values (1, 'Alice Again')")) {
        return 1;
    }
    if (!exec_ok(db, "use SCHOOL")) {
        return 1;
    }
    if (!exec_ok(db, "insert into grades values (2, 'B')")) {
        return 1;
    }
    if (!exec_fails(db, "insert into grades values (2, 'B again')")) {
        return 1;
    }
    if (!exec_ok(db, "drop table grades")) {
        return 1;
    }
    if (!exec_ok(db, "drop table if exists grades")) {
        return 1;
    }
    if (!exec_fails(db, "select * from grades")) {
        return 1;
    }
    if (!exec_ok(db, "create table grades(STUDENT_ID INT, GRADE VARCHAR, PRIMARY KEY(STUDENT_ID))")) {
        return 1;
    }
    if (!exec_ok(db, "insert into grades values (2, 'B')")) {
        return 1;
    }
    if (!exec_ok(db, "use default")) {
        return 1;
    }

    Capture capture;
    if (!exec_ok(db, "select NAME from student where ID = 1", &capture)) {
        return 1;
    }
    if (capture.rows.size() != 1 || capture.rows[0][0] != "Alice") {
        std::cerr << "unexpected SELECT result\n";
        return 1;
    }
    Capture greater_than_capture;
    if (!exec_ok(db, "select NAME from student where ID > 1", &greater_than_capture)) {
        return 1;
    }
    if (greater_than_capture.rows.size() != 3 ||
        !contains_single_value(greater_than_capture, "Bob") ||
        !contains_single_value(greater_than_capture, "Cara") ||
        !contains_single_value(greater_than_capture, "Eli")) {
        std::cerr << "unexpected comparison SELECT result\n";
        return 1;
    }
    Capture batched_insert_capture;
    if (!exec_ok(db, "select NAME from student where ID > 2", &batched_insert_capture)) {
        return 1;
    }
    if (batched_insert_capture.rows.size() != 2 ||
        !contains_single_value(batched_insert_capture, "Cara") ||
        !contains_single_value(batched_insert_capture, "Eli")) {
        std::cerr << "unexpected batched INSERT result\n";
        return 1;
    }

    Capture school_grade_capture;
    if (!exec_ok(db, "select GRADE from school.grades where STUDENT_ID = 2", &school_grade_capture)) {
        return 1;
    }
    if (school_grade_capture.rows.size() != 1 || school_grade_capture.rows[0][0] != "B") {
        std::cerr << "unexpected PRIMARY KEY select result\n";
        return 1;
    }

    Capture join_capture;
    if (!exec_ok(
            db,
            "SeLeCt student.NAME, grades.GRADE FrOm StUdEnT InNeR JoIn GrAdEs On student.ID = grades.STUDENT_ID",
            &join_capture)) {
        return 1;
    }
    if (join_capture.rows.size() != 1 || join_capture.rows[0][0] != "Alice" || join_capture.rows[0][1] != "A") {
        std::cerr << "unexpected JOIN result\n";
        return 1;
    }

    Capture qualified_join_where_capture;
    if (!exec_ok(
            db,
            "SELECT student.NAME, grades.GRADE FROM STUDENT INNER JOIN GRADES ON student.ID = grades.STUDENT_ID "
            "WHERE grades.GRADE = 'A'",
            &qualified_join_where_capture)) {
        return 1;
    }
    if (qualified_join_where_capture.rows.size() != 1 || qualified_join_where_capture.rows[0][0] != "Alice") {
        std::cerr << "unexpected qualified JOIN WHERE result\n";
        return 1;
    }

    Capture multi_join_capture;
    if (!exec_ok(
            db,
            "SELECT student.NAME, grades.GRADE, attendance.STATUS "
            "FROM student "
            "INNER JOIN grades ON student.ID = grades.STUDENT_ID "
            "INNER JOIN attendance ON grades.STUDENT_ID = attendance.STUDENT_ID",
            &multi_join_capture)) {
        return 1;
    }
    if (multi_join_capture.rows.size() != 1 ||
        multi_join_capture.rows[0][0] != "Alice" ||
        multi_join_capture.rows[0][1] != "A" ||
        multi_join_capture.rows[0][2] != "Present") {
        std::cerr << "unexpected multi JOIN result\n";
        return 1;
    }

    if (!exec_fails(db, "select missing_column from student")) {
        return 1;
    }

    if (!exec_ok(db, "create table events(AT DATETIME PRIMARY KEY, NAME VARCHAR)")) {
        return 1;
    }
    if (!exec_ok(db, "create table metrics(ID INT PRIMARY KEY, VALUE DECIMAL(10,2), LABEL VARCHAR(255))")) {
        return 1;
    }
    if (!exec_ok(db, "insert into metrics values (1, 12.50, 'ok')")) {
        return 1;
    }
    if (!exec_ok(db, "insert into events values ('2026-03-24 10:00:00', 'Launch')")) {
        return 1;
    }
    Capture metrics_capture;
    if (!exec_ok(db, "select LABEL from metrics where VALUE > 10.0", &metrics_capture)) {
        return 1;
    }
    if (metrics_capture.rows.size() != 1 || metrics_capture.rows[0][0] != "ok") {
        std::cerr << "unexpected decimal comparison result\n";
        return 1;
    }
    if (!exec_fails(db, "insert into events values ('not-a-date', 'Broken')")) {
        return 1;
    }

    if (!exec_ok(db, "create table sessions(ID INT PRIMARY KEY, NAME VARCHAR)")) {
        return 1;
    }
    if (!exec_ok(db, "insert into sessions values (1, 'Expired') expires at '2000-01-01 00:00:00'")) {
        return 1;
    }
    Capture expired_capture;
    if (!exec_ok(db, "select * from sessions", &expired_capture)) {
        return 1;
    }
    if (!expired_capture.rows.empty()) {
        std::cerr << "expired row should not be returned\n";
        return 1;
    }

    flexql_close(db);
    std::cout << "All tests passed\n";
    return 0;
}
