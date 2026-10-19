#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>

#include "db/DatabaseManager.h"
#include "duckdb.h"
#include "simdjson.h"

using namespace restfulEz;

static const std::string TEST_DIR = "./test_m3_adv_ws";
static const std::string TEST_DB_PATH = "./test_m3_adv_ws/db/ezreal_data.duckdb";
static const std::string TEST_EXPORTS_DIR = "./output/exports";

static void ResetWorkspace() {
    std::error_code ec;
    std::filesystem::remove_all(TEST_DIR, ec);
    std::filesystem::create_directories(TEST_DIR + "/db");
    std::filesystem::create_directories(TEST_EXPORTS_DIR);
}

// Sample Match JSON
static std::string CreateSampleMatchJson(
    const std::string& match_id,
    const std::vector<std::pair<std::string, std::string>>& participants
) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"metadata\": {\"matchId\": \"" << match_id << "\"},\n"
       << "  \"info\": {\n"
       << "    \"gameCreation\": 1700000000000,\n"
       << "    \"gameDuration\": 1800,\n"
       << "    \"gameVersion\": \"14.10.1\",\n"
       << "    \"queueId\": 420,\n"
       << "    \"participants\": [\n";

    for (size_t i = 0; i < participants.size(); ++i) {
        int part_id = static_cast<int>(i + 1);
        int team_id = (i < participants.size() / 2) ? 100 : 200;
        bool win = (team_id == 100);
        ss << "      {\n"
           << "        \"participantId\": " << part_id << ",\n"
           << "        \"puuid\": \"" << participants[i].first << "\",\n"
           << "        \"riotIdGameName\": \"" << participants[i].first << "_Name\",\n"
           << "        \"riotIdTagline\": \"KR1\",\n"
           << "        \"championName\": \"" << participants[i].second << "\",\n"
           << "        \"championId\": " << (100 + part_id) << ",\n"
           << "        \"teamId\": " << team_id << ",\n"
           << "        \"win\": " << (win ? "true" : "false") << ",\n"
           << "        \"kills\": " << (part_id * 2) << ",\n"
           << "        \"deaths\": " << (part_id % 3) << ",\n"
           << "        \"assists\": " << (part_id + 3) << "\n"
           << "      }" << (i + 1 < participants.size() ? "," : "") << "\n";
    }
    ss << "    ]\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

// Sample Timeline JSON
static std::string CreateSampleTimelineJson(
    const std::string& match_id,
    int participant_count = 4,
    int frame_count = 5
) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"metadata\": {\"matchId\": \"" << match_id << "\"},\n"
       << "  \"info\": {\n"
       << "    \"frameInterval\": 60000,\n"
       << "    \"frames\": [\n";

    for (int f = 0; f < frame_count; ++f) {
        ss << "      {\n"
           << "        \"timestamp\": " << (f * 60000) << ",\n"
           << "        \"participantFrames\": {\n";

        for (int p = 1; p <= participant_count; ++p) {
            ss << "          \"" << p << "\": {\n"
               << "            \"participantId\": " << p << ",\n"
               << "            \"totalGold\": " << (500 + f * 350 * p) << ",\n"
               << "            \"currentGold\": " << (100 + f * 50) << ",\n"
               << "            \"xp\": " << (f * 400 * p) << ",\n"
               << "            \"level\": " << (1 + f) << ",\n"
               << "            \"minionsKilled\": " << (f * 8 * p) << ",\n"
               << "            \"jungleMinionsKilled\": " << (f * 2) << ",\n"
               << "            \"position\": {\"x\": " << (1000 + f * 100) << ", \"y\": " << (2000 + f * 100) << "},\n"
               << "            \"damageStats\": {\"totalDamageDoneToChampions\": " << (f * 500 * p) << "}\n"
               << "          }" << (p < participant_count ? "," : "") << "\n";
        }
        ss << "        },\n"
           << "        \"events\": [\n"
           << "          {\n"
           << "            \"type\": \"CHAMPION_KILL\",\n"
           << "            \"timestamp\": " << (f * 60000 + 15000) << ",\n"
           << "            \"killerId\": 1,\n"
           << "            \"victimId\": 2,\n"
           << "            \"position\": {\"x\": 1500, \"y\": 2500},\n"
           << "            \"assistingParticipantIds\": [3]\n"
           << "          }\n"
           << "        ]\n"
           << "      }" << (f + 1 < frame_count ? "," : "") << "\n";
    }
    ss << "    ]\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

// =============================================================================
// TEST SUITE 1: SQL Security & Mutation Prevention
// =============================================================================
static void TestSuite1_SQLSecurity(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 1] Adversarial SQL Security & Mutation Rejection" << std::endl;
    std::cout << "========================================================" << std::endl;

    int64_t initial_matches = db.GetMatchCount();
    int64_t initial_participants = db.GetParticipantCount();
    assert(initial_matches > 0);
    assert(initial_participants > 0);

    // 1. Direct mutation keywords at start
    const std::vector<std::pair<std::string, std::string>> direct_mutations = {
        {"DROP TABLE", "DROP TABLE matches;"},
        {"DROP VIEW", "DROP VIEW IF EXISTS some_view;"},
        {"DELETE FROM", "DELETE FROM matches;"},
        {"DELETE FROM participants", "DELETE FROM match_participants;"},
        {"UPDATE", "UPDATE matches SET queue_id = 999;"},
        {"INSERT INTO", "INSERT INTO matches VALUES ('hacked', 0, 0, '1.0', 0, '{}');"},
        {"ATTACH", "ATTACH ':memory:' AS evil_db;"},
        {"DETACH", "DETACH evil_db;"},
        {"ALTER TABLE", "ALTER TABLE matches ADD COLUMN hacked INTEGER;"},
        {"CREATE TABLE", "CREATE TABLE evil_table (id INT);"},
        {"TRUNCATE", "TRUNCATE matches;"},
        {"REPLACE INTO", "REPLACE INTO matches VALUES ('hacked', 0, 0, '1.0', 0, '{}');"},
        {"GRANT", "GRANT ALL ON ALL TABLES TO public;"},
        {"REVOKE", "REVOKE ALL ON ALL TABLES FROM public;"},
        {"VACUUM", "VACUUM;"}
    };

    for (const auto& [label, query] : direct_mutations) {
        QueryResult res = db.ExecuteQuery(query);
        assert(!res.success);
        assert(!res.error_message.empty());
        assert(res.row_count == 0);
        std::cout << "  [PASS] Direct mutation rejected: " << label << std::endl;
    }

    // 2. Case variations (mixed case / lowercase)
    const std::vector<std::string> case_variations = {
        "dRoP tAbLe matches;",
        "DeLeTe FrOm matches;",
        "uPdAtE matches SeT queue_id = 999;",
        "InSeRt InTo matches VALUES ('h', 0, 0, '1', 0, '{}');",
        "aTtAcH ':memory:' AS evil;",
        "dEtAcH evil;",
        "CrEaTe TaBlE evil (x INT);",
        "AlTeR tAbLe matches ADD COLUMN x INT;",
        "tRuNcAtE matches;",
        "vAcUuM;"
    };

    for (const auto& q : case_variations) {
        QueryResult res = db.ExecuteQuery(q);
        assert(!res.success);
        assert(!res.error_message.empty());
    }
    std::cout << "  [PASS] Case-insensitive mutation keywords rejected." << std::endl;

    // 3. Multi-statement attacks with semicolons
    const std::vector<std::string> multi_stmt_attacks = {
        "SELECT 1; DROP TABLE matches;",
        "SELECT * FROM matches; DELETE FROM matches;",
        "SELECT 1; ; ; DROP TABLE matches;",
        "SELECT 1;\n\tDROP TABLE matches;",
        "SELECT 1; INSERT INTO matches VALUES ('evil', 0, 0, '1', 0, '{}');",
        "SELECT 1; UPDATE match_participants SET kills = 999;",
        "SELECT 1; ATTACH ':memory:' AS pwned;",
        "SELECT 1; DETACH pwned;",
        "SELECT 1; ALTER TABLE matches ADD COLUMN hacked INT;",
        "SELECT 1; CREATE TABLE pwned (x INT);",
        "SELECT 1; TRUNCATE matches;",
        "SELECT 1; VACUUM;"
    };

    for (const auto& q : multi_stmt_attacks) {
        QueryResult res = db.ExecuteQuery(q);
        assert(!res.success);
        assert(!res.error_message.empty());
    }
    std::cout << "  [PASS] Semicolon multi-statement mutation attacks rejected." << std::endl;

    // 4. CTE & Subquery mutation attacks
    const std::vector<std::string> nested_mutation_attacks = {
        "WITH del AS (DELETE FROM matches RETURNING *) SELECT * FROM del;",
        "WITH ins AS (INSERT INTO matches VALUES ('hacked', 0, 0, '1.0', 0, '{}') RETURNING *) SELECT * FROM ins;",
        "WITH upd AS (UPDATE matches SET queue_id = 1 RETURNING *) SELECT * FROM upd;",
        "SELECT * FROM (DELETE FROM matches RETURNING *);"
    };

    for (const auto& q : nested_mutation_attacks) {
        QueryResult res = db.ExecuteQuery(q);
        assert(!res.success);
        assert(!res.error_message.empty());
    }
    std::cout << "  [PASS] CTE & nested subquery mutation attacks rejected." << std::endl;

    // 5. Non-read-only statements (not starting with SELECT, WITH, PRAGMA, EXPLAIN)
    const std::vector<std::string> other_forbidden_statements = {
        "COPY matches TO '/tmp/pwned.csv';",
        "EXPORT DATABASE '/tmp/db_exp';",
        "IMPORT DATABASE '/tmp/db_imp';",
        "SET threads = 1;",
        "CALL duckdb_tables();",
        "INSTALL spatial;",
        "LOAD spatial;",
        "CHECKPOINT;"
    };

    for (const auto& q : other_forbidden_statements) {
        QueryResult res = db.ExecuteQuery(q);
        assert(!res.success);
        assert(!res.error_message.empty());
    }
    std::cout << "  [PASS] Non-read-only statements (COPY, EXPORT, SET, etc.) rejected." << std::endl;

    // 6. Verify database state is 100% UNTOUCHED
    assert(db.GetMatchCount() == initial_matches);
    assert(db.GetParticipantCount() == initial_participants);
    std::cout << "  [PASS] Database state integrity confirmed: zero data corruption or modification." << std::endl;
}

// =============================================================================
// TEST SUITE 2: Query Parser Boundaries, Comments, Whitespace & Error Handling
// =============================================================================
static void TestSuite2_ParserBoundariesAndErrors(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 2] Parser Boundaries, Comments, Whitespace & Errors" << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. Permitted queries containing forbidden words inside string literals or identifiers
    {
        QueryResult r1 = db.ExecuteQuery("SELECT 'DROP TABLE matches;' AS drop_str;");
        assert(r1.success);
        assert(r1.row_count == 1);
        assert(r1.rows[0][0] == "DROP TABLE matches;");

        QueryResult r2 = db.ExecuteQuery("SELECT 'INSERT INTO' AS ins, 'DELETE FROM' AS del;");
        assert(r2.success);
        assert(r2.row_count == 1);
        assert(r2.rows[0][0] == "INSERT INTO");
        assert(r2.rows[0][1] == "DELETE FROM");

        QueryResult r3 = db.ExecuteQuery("SELECT 'It''s a DROP test' AS escaped_quote;");
        assert(r3.success);
        assert(r3.row_count == 1);
        assert(r3.rows[0][0] == "It's a DROP test");

        QueryResult r4 = db.ExecuteQuery("SELECT * FROM match_participants WHERE champion_name = 'DROP';");
        assert(r4.success);
        assert(r4.row_count == 0); // No champion named DROP

        QueryResult r5 = db.ExecuteQuery("SELECT match_id AS drop_table_col FROM matches;");
        assert(r5.success);
        assert(r5.row_count > 0);
        assert(r5.column_names[0] == "drop_table_col");

        std::cout << "  [PASS] Permitted queries with keywords in literals/identifiers succeed." << std::endl;
    }

    // 2. Comments handling
    {
        QueryResult c1 = db.ExecuteQuery("-- Single line comment\nSELECT 1 AS num;");
        assert(c1.success);
        assert(c1.rows[0][0] == "1");

        QueryResult c2 = db.ExecuteQuery("/* Block comment */ SELECT 2 AS num;");
        assert(c2.success);
        assert(c2.rows[0][0] == "2");

        QueryResult c3 = db.ExecuteQuery("SELECT 3 AS num; -- Trailing comment");
        assert(c3.success);
        assert(c3.rows[0][0] == "3");

        QueryResult c4 = db.ExecuteQuery("SELECT 2 /* Inline comment */ + 2 AS sum;");
        assert(c4.success);
        assert(c4.rows[0][0] == "4");

        QueryResult c5 = db.ExecuteQuery("SELECT /* DROP TABLE matches; */ 5 AS num;");
        assert(c5.success);
        assert(c5.rows[0][0] == "5");

        QueryResult c6 = db.ExecuteQuery("SELECT 6 AS num; /* DROP TABLE matches; */");
        assert(c6.success);
        assert(c6.rows[0][0] == "6");

        // Comment with newline and semicolon
        QueryResult c7 = db.ExecuteQuery("-- comment line 1\n-- comment line 2\nSELECT 7 AS num;");
        assert(c7.success);
        assert(c7.rows[0][0] == "7");

        std::cout << "  [PASS] Complex comments and comment stripping verified." << std::endl;
    }

    // 3. Multi-statement permitted queries & trailing semicolons
    {
        QueryResult m1 = db.ExecuteQuery("SELECT 1; SELECT 2;");
        assert(m1.success);

        QueryResult m2 = db.ExecuteQuery("SELECT 1;;;;");
        assert(m2.success);
        assert(m2.rows[0][0] == "1");

        std::cout << "  [PASS] Trailing semicolons and multi-statement SELECT handled cleanly." << std::endl;
    }

    // 4. Empty, whitespace-only, comment-only queries
    const std::vector<std::string> empty_queries = {
        "",
        "   ",
        "\t\r\n   ",
        ";;;;",
        "-- only line comment\n",
        "/* only block comment */",
        "-- comment 1\n/* comment 2 */ ; ; ;"
    };

    for (const auto& eq : empty_queries) {
        QueryResult res = db.ExecuteQuery(eq);
        assert(!res.success);
        assert(res.error_message.find("Empty SQL query") != std::string::npos);
    }
    std::cout << "  [PASS] Empty, whitespace-only, and comment-only queries safely rejected." << std::endl;

    // 5. Syntax errors
    const std::vector<std::string> syntax_errors = {
        "SELECT FROM WHERE;",
        "SELECT 1 + ;",
        "SELECT * FORM matches;",
        "SELECT (1 + 2;"
    };

    for (const auto& se : syntax_errors) {
        QueryResult res = db.ExecuteQuery(se);
        assert(!res.success);
        assert(!res.error_message.empty());
    }
    std::cout << "  [PASS] Syntax errors cleanly caught with error message." << std::endl;

    // 6. Non-existent objects
    {
        QueryResult ne_tbl = db.ExecuteQuery("SELECT * FROM nonexistent_table_xyz_123;");
        assert(!ne_tbl.success);
        assert(ne_tbl.error_message.find("does not exist") != std::string::npos ||
               ne_tbl.error_message.find("not found") != std::string::npos);

        QueryResult ne_col = db.ExecuteQuery("SELECT nonexistent_col_abc FROM matches;");
        assert(!ne_col.success);
        assert(ne_col.error_message.find("not found") != std::string::npos ||
               ne_col.error_message.find("does not exist") != std::string::npos);

        std::cout << "  [PASS] Non-existent tables and columns cleanly reported." << std::endl;
    }
}

// =============================================================================
// TEST SUITE 3: Complex Queries, Relational JOINs, Aggregations & Window Functions
// =============================================================================
static void TestSuite3_ComplexQueries(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 3] Complex Queries, Relational JOINs & Analytics" << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. Relational 3-table JOIN across matches, match_participants, and timeline_participant_frames
    std::string join_sql = R"(
        SELECT
            m.match_id,
            p.champion_name,
            p.kills,
            t.frame_index,
            t.total_gold,
            t.xp
        FROM match_participants p
        JOIN matches m ON p.match_id = m.match_id
        JOIN timeline_participant_frames t
            ON p.match_id = t.match_id AND p.participant_id = t.participant_id
        WHERE t.frame_index = 2
        ORDER BY t.total_gold DESC;
    )";

    QueryResult join_res = db.ExecuteQuery(join_sql);
    assert(join_res.success);
    assert(join_res.row_count > 0);
    assert(join_res.column_names.size() == 6);
    assert(join_res.column_names[0] == "match_id");
    assert(join_res.column_names[1] == "champion_name");
    assert(join_res.column_names[4] == "total_gold");
    std::cout << "  [PASS] 3-Table Relational JOIN executed (" << join_res.row_count << " rows, "
              << join_res.execution_time_ms << " ms)." << std::endl;

    // 2. Aggregations with GROUP BY, HAVING, CASE WHEN
    std::string agg_sql = R"(
        SELECT
            champion_name,
            COUNT(*) AS picks,
            SUM(CASE WHEN win THEN 1 ELSE 0 END) AS wins,
            ROUND(AVG(kills), 2) AS avg_k,
            ROUND(AVG(deaths), 2) AS avg_d,
            ROUND(AVG(assists), 2) AS avg_a
        FROM match_participants
        GROUP BY champion_name
        HAVING picks > 0
        ORDER BY picks DESC;
    )";

    QueryResult agg_res = db.ExecuteQuery(agg_sql);
    assert(agg_res.success);
    assert(agg_res.row_count > 0);
    assert(agg_res.column_names.size() == 6);
    assert(agg_res.column_names[0] == "champion_name");
    assert(agg_res.column_names[1] == "picks");
    std::cout << "  [PASS] Complex GROUP BY / HAVING / Aggregation query executed." << std::endl;

    // 3. Window functions: running average and partition rank
    std::string window_sql = R"(
        SELECT
            match_id,
            participant_id,
            frame_index,
            total_gold,
            AVG(total_gold) OVER (
                PARTITION BY match_id, participant_id
                ORDER BY frame_index
                ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
            ) AS running_avg_gold,
            RANK() OVER (
                PARTITION BY match_id, frame_index
                ORDER BY total_gold DESC
            ) AS frame_gold_rank
        FROM timeline_participant_frames
        ORDER BY match_id, frame_index, frame_gold_rank;
    )";

    QueryResult win_res = db.ExecuteQuery(window_sql);
    assert(win_res.success);
    assert(win_res.row_count > 0);
    assert(win_res.column_names.size() == 6);
    assert(win_res.column_names[4] == "running_avg_gold");
    assert(win_res.column_names[5] == "frame_gold_rank");
    std::cout << "  [PASS] Window functions (running average & rank partitioning) executed." << std::endl;

    // 4. CTE (WITH query)
    std::string cte_sql = R"(
        WITH top_gold_per_frame AS (
            SELECT
                match_id,
                frame_index,
                participant_id,
                total_gold,
                RANK() OVER (PARTITION BY match_id, frame_index ORDER BY total_gold DESC) AS rnk
            FROM timeline_participant_frames
        )
        SELECT match_id, frame_index, participant_id, total_gold
        FROM top_gold_per_frame
        WHERE rnk = 1
        ORDER BY frame_index;
    )";

    QueryResult cte_res = db.ExecuteQuery(cte_sql);
    assert(cte_res.success);
    assert(cte_res.row_count > 0);
    assert(cte_res.column_names[0] == "match_id");
    assert(cte_res.column_names[3] == "total_gold");
    std::cout << "  [PASS] Common Table Expression (WITH clause) executed." << std::endl;

    // 5. PRAGMA inspection queries
    QueryResult pragma_res = db.ExecuteQuery("PRAGMA table_info('matches');");
    assert(pragma_res.success);
    assert(pragma_res.row_count >= 5);
    std::cout << "  [PASS] PRAGMA table_info executed." << std::endl;

    // 6. EXPLAIN queries
    QueryResult explain_res = db.ExecuteQuery("EXPLAIN SELECT * FROM matches;");
    assert(explain_res.success);
    assert(explain_res.row_count > 0);
    std::cout << "  [PASS] EXPLAIN query execution plan retrieved." << std::endl;
}

// =============================================================================
// TEST SUITE 4: CSV Export Verification & RFC-4180 Escaping
// =============================================================================
static void TestSuite4_CSVExporter(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 4] CSV Exporter Verification & RFC-4180 Escaping" << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. ExportQueryToCSV: normal analytical query
    std::string export_csv1 = TEST_EXPORTS_DIR + "/adv_champions.csv";
    std::string q_champ = "SELECT champion_name, kills, deaths, assists FROM match_participants ORDER BY kills DESC;";
    assert(db.ExportQueryToCSV(q_champ, export_csv1));
    assert(std::filesystem::exists(export_csv1));
    assert(std::filesystem::file_size(export_csv1) > 0);

    // Verify header and contents
    {
        std::ifstream in(export_csv1);
        assert(in.is_open());
        std::string header;
        std::getline(in, header);
        assert(header == "champion_name,kills,deaths,assists");
        int row_count = 0;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) row_count++;
        }
        assert(row_count == db.GetParticipantCount());
        std::cout << "  [PASS] ExportQueryToCSV created valid file (" << row_count << " rows)." << std::endl;
    }

    // 2. ExportTableToCSV for each core table
    const std::vector<std::string> core_tables = {
        "matches", "match_participants", "timeline_participant_frames", "timeline_events", "players"
    };

    for (const auto& tbl : core_tables) {
        std::string tbl_csv = TEST_EXPORTS_DIR + "/" + tbl + "_table_dump.csv";
        assert(db.ExportTableToCSV(tbl, tbl_csv));
        assert(std::filesystem::exists(tbl_csv));
        assert(std::filesystem::file_size(tbl_csv) > 0);

        std::ifstream in(tbl_csv);
        assert(in.is_open());
        std::string header;
        std::getline(in, header);
        assert(!header.empty());
        std::cout << "  [PASS] ExportTableToCSV (" << tbl << ") dumped to " << tbl_csv << std::endl;
    }

    // 3. RFC-4180 Escaping: Commas, Quotes, Newlines
    std::string escape_query = R"(
        SELECT
            'Normal' AS col1,
            'Value, with, commas' AS col2,
            'Value "with" quotes' AS col3,
            'Value' || chr(10) || 'with newline' AS col4;
    )";

    std::string escape_csv = TEST_EXPORTS_DIR + "/rfc4180_escape_test.csv";
    assert(db.ExportQueryToCSV(escape_query, escape_csv));
    assert(std::filesystem::exists(escape_csv));

    {
        std::ifstream in(escape_csv);
        assert(in.is_open());
        std::string header;
        std::getline(in, header);
        assert(header == "col1,col2,col3,col4");

        // Read entire remaining content
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string body = buffer.str();

        // Must contain escaped cell with commas: "Value, with, commas"
        assert(body.find("\"Value, with, commas\"") != std::string::npos);
        // Must contain escaped cell with doubled quotes: "Value ""with"" quotes"
        assert(body.find("\"Value \"\"with\"\" quotes\"") != std::string::npos);
        // Must contain escaped cell with newline: "Value\nwith newline"
        assert(body.find("\"Value\nwith newline\"") != std::string::npos);

        std::cout << "  [PASS] RFC-4180 Escaping (commas, doubled quotes, newlines) 100% compliant." << std::endl;
    }

    // 4. Rejection of invalid/malicious table names in ExportTableToCSV
    std::string bad_csv = TEST_EXPORTS_DIR + "/should_not_exist.csv";
    assert(!db.ExportTableToCSV("", bad_csv));
    assert(!db.ExportTableToCSV("matches; DROP TABLE players;", bad_csv));
    assert(!db.ExportTableToCSV("matches--", bad_csv));
    assert(!db.ExportTableToCSV("matches/*", bad_csv));
    assert(!db.ExportTableToCSV("matches '", bad_csv));
    assert(!db.ExportTableToCSV("matches\"", bad_csv));
    assert(!db.ExportTableToCSV("nonexistent_table_99999", bad_csv));
    assert(!std::filesystem::exists(bad_csv));
    std::cout << "  [PASS] Invalid and malicious table names rejected in ExportTableToCSV." << std::endl;

    // 5. Rejection of mutating or invalid queries in ExportQueryToCSV
    assert(!db.ExportQueryToCSV("DROP TABLE matches;", bad_csv));
    assert(!db.ExportQueryToCSV("DELETE FROM matches;", bad_csv));
    assert(!db.ExportQueryToCSV("SELECT FROM WHERE;", bad_csv));
    assert(!db.ExportQueryToCSV("", bad_csv));
    assert(!std::filesystem::exists(bad_csv));
    std::cout << "  [PASS] Mutating and invalid queries rejected in ExportQueryToCSV (no ghost files created)." << std::endl;
}

// =============================================================================
// TEST SUITE 5: Summary Query Methods & Telemetry Accuracy
// =============================================================================
static void TestSuite5_SummariesAndTelemetry(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 5] Summary Query Methods & Telemetry Accuracy" << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. GetTableCounts
    TableCounts tc = db.GetTableCounts();
    assert(tc.players >= 4);
    assert(tc.matches >= 2);
    assert(tc.match_participants >= 8);
    assert(tc.match_timelines >= 2);
    assert(tc.timeline_participant_frames >= 20);
    assert(tc.timeline_events >= 10);
    assert(db.GetParticipantCount() == tc.match_participants);
    assert(db.GetTimelineFrameCount() == tc.timeline_participant_frames);
    std::cout << "  [PASS] GetTableCounts: players=" << tc.players
              << " matches=" << tc.matches
              << " participants=" << tc.match_participants
              << " timelines=" << tc.match_timelines
              << " frames=" << tc.timeline_participant_frames
              << " events=" << tc.timeline_events << std::endl;

    // 2. Champion Performance Summary
    QueryResult champ_res = db.GetChampionPerformanceSummary(10);
    assert(champ_res.success);
    assert(champ_res.row_count > 0);
    assert(champ_res.column_names.size() == 8);
    assert(champ_res.column_names[0] == "Champion");
    assert(champ_res.column_names[1] == "Picks");
    assert(champ_res.column_names[2] == "Wins");
    assert(champ_res.column_names[3] == "Winrate %");
    assert(champ_res.column_names[7] == "KDA");
    std::cout << "  [PASS] GetChampionPerformanceSummary verified." << std::endl;

    // 3. Top Tracked Players Summary
    QueryResult player_res = db.GetTopTrackedPlayersSummary(10);
    assert(player_res.success);
    assert(player_res.row_count > 0);
    assert(player_res.column_names.size() == 7);
    assert(player_res.column_names[0] == "Riot ID");
    assert(player_res.column_names[1] == "Matches in DB");
    assert(player_res.column_names[2] == "Wins");
    assert(player_res.column_names[3] == "Winrate %");
    std::cout << "  [PASS] GetTopTrackedPlayersSummary verified." << std::endl;

    // 4. Timeline Frames Progression Summary
    QueryResult tf_res = db.GetTimelineFramesSummary();
    assert(tf_res.success);
    assert(tf_res.row_count > 0);
    assert(tf_res.column_names.size() == 6);
    assert(tf_res.column_names[0] == "Minute");
    assert(tf_res.column_names[1] == "Avg Gold");
    assert(tf_res.column_names[2] == "Avg XP");
    std::cout << "  [PASS] GetTimelineFramesSummary verified." << std::endl;

    // 5. Timeline Events Summary
    QueryResult ev_res = db.GetTimelineEventsSummary();
    assert(ev_res.success);
    assert(ev_res.row_count > 0);
    assert(ev_res.column_names.size() == 4);
    assert(ev_res.column_names[0] == "Event Type");
    assert(ev_res.column_names[1] == "Count");
    std::cout << "  [PASS] GetTimelineEventsSummary verified." << std::endl;
}

// =============================================================================
// TEST SUITE 6: Concurrency & Stress Testing
// =============================================================================
static void TestSuite6_ConcurrencyStress(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 6] Concurrency, Thread-Safety & Mutation Pressure" << std::endl;
    std::cout << "========================================================" << std::endl;

    std::atomic<bool> stop_flag{false};
    std::atomic<int64_t> read_success_count{0};
    std::atomic<int64_t> mutation_rejection_count{0};
    std::atomic<int64_t> errors{0};

    // 6 Reader threads executing analytical queries concurrently
    std::vector<std::thread> readers;
    for (int t = 0; t < 6; ++t) {
        readers.emplace_back([&db, &stop_flag, &read_success_count, &errors, t]() {
            int q_idx = 0;
            while (!stop_flag.load()) {
                std::string sql;
                if (q_idx % 4 == 0) {
                    sql = "SELECT match_id, queue_id FROM matches;";
                } else if (q_idx % 4 == 1) {
                    sql = "SELECT champion_name, AVG(kills) FROM match_participants GROUP BY champion_name;";
                } else if (q_idx % 4 == 2) {
                    sql = "SELECT frame_index, AVG(total_gold) FROM timeline_participant_frames GROUP BY frame_index;";
                } else {
                    sql = "PRAGMA table_info('match_participants');";
                }

                QueryResult qr = db.ExecuteQuery(sql);
                if (qr.success) {
                    read_success_count++;
                } else {
                    errors++;
                }
                q_idx++;
                std::this_thread::yield();
            }
        });
    }

    // 2 Attacker threads attempting mutations concurrently
    std::vector<std::thread> attackers;
    for (int t = 0; t < 2; ++t) {
        attackers.emplace_back([&db, &stop_flag, &mutation_rejection_count, &errors, t]() {
            int q_idx = 0;
            while (!stop_flag.load()) {
                std::string attack_sql;
                if (q_idx % 3 == 0) {
                    attack_sql = "DROP TABLE matches;";
                } else if (q_idx % 3 == 1) {
                    attack_sql = "DELETE FROM match_participants;";
                } else {
                    attack_sql = "SELECT 1; DROP TABLE timeline_events;";
                }

                QueryResult qr = db.ExecuteQuery(attack_sql);
                if (!qr.success) {
                    mutation_rejection_count++;
                } else {
                    errors++; // Mutation succeeded! Critical failure!
                }
                q_idx++;
                std::this_thread::yield();
            }
        });
    }

    // Run concurrency test for 500 ms
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_flag.store(true);

    for (auto& r : readers) r.join();
    for (auto& a : attackers) a.join();

    assert(errors.load() == 0);
    assert(read_success_count.load() > 50);
    assert(mutation_rejection_count.load() > 50);

    // Verify database state is intact
    assert(db.GetMatchCount() >= 2);
    assert(db.GetParticipantCount() >= 8);

    std::cout << "  [PASS] Concurrent stress: " << read_success_count.load()
              << " read queries succeeded, " << mutation_rejection_count.load()
              << " mutation attacks rejected with 0 errors/crashes." << std::endl;
}

// =============================================================================
// TEST SUITE 7: Uninitialized Guard Tests
// =============================================================================
static void TestSuite7_UninitializedGuards(DatabaseManager& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Suite 7] DatabaseManager Uninitialized Safety Guards" << std::endl;
    std::cout << "========================================================" << std::endl;

    db.Close();
    assert(!db.IsInitialized());

    // ExecuteQuery
    QueryResult qr = db.ExecuteQuery("SELECT 1;");
    assert(!qr.success);
    assert(qr.error_message == "Database is not initialized.");

    // Table counts
    TableCounts tc = db.GetTableCounts();
    assert(tc.matches == 0);
    assert(tc.players == 0);
    assert(tc.timeline_participant_frames == 0);

    // Summaries
    assert(!db.GetChampionPerformanceSummary().success);
    assert(!db.GetTopTrackedPlayersSummary().success);
    assert(!db.GetTimelineFramesSummary().success);
    assert(!db.GetTimelineEventsSummary().success);

    // CSV Exports
    std::string csv_path = TEST_EXPORTS_DIR + "/uninitialized.csv";
    assert(!db.ExportQueryToCSV("SELECT 1;", csv_path));
    assert(!db.ExportTableToCSV("matches", csv_path));
    assert(!std::filesystem::exists(csv_path));

    std::cout << "  [PASS] All uninitialized DatabaseManager calls safely guarded." << std::endl;
}

// =============================================================================
// MAIN ENTRYPOINT
// =============================================================================
int main() {
    std::cout << "=====================================================================" << std::endl;
    std::cout << "[Adversarial Test] Milestone 3 SQL Engine & CSV Exporter Challenge" << std::endl;
    std::cout << "=====================================================================" << std::endl;

    ResetWorkspace();

    DatabaseManager& db = DatabaseManager::GetInstance();
    assert(db.Init(TEST_DB_PATH));
    assert(db.IsInitialized());

    // Seed test database
    std::cout << "\n[Setup] Seeding test database with matches and timelines..." << std::endl;
    db.ResolveOrCreatePlayer("DEV_A", "PUUID_FAKER", "Faker", "KR1");
    db.ResolveOrCreatePlayer("DEV_A", "PUUID_GUMAYUSI", "Gumayusi", "KR1");
    db.ResolveOrCreatePlayer("DEV_A", "PUUID_KERIA", "Keria", "KR1");
    db.ResolveOrCreatePlayer("DEV_A", "PUUID_ZEUS", "Zeus", "KR1");

    std::vector<std::pair<std::string, std::string>> parts1 = {
        {"PUUID_FAKER", "Azir"},
        {"PUUID_GUMAYUSI", "Varus"},
        {"PUUID_KERIA", "Nami"},
        {"PUUID_ZEUS", "Aatrox"}
    };
    std::string m1_json = CreateSampleMatchJson("KR_1001", parts1);
    assert(db.SaveMatch("KR_1001", m1_json));
    std::string t1_json = CreateSampleTimelineJson("KR_1001", 4, 5);
    assert(db.SaveTimeline("KR_1001", t1_json));

    std::vector<std::pair<std::string, std::string>> parts2 = {
        {"PUUID_FAKER", "Orianna"},
        {"PUUID_GUMAYUSI", "Jinx"},
        {"PUUID_KERIA", "Thresh"},
        {"PUUID_ZEUS", "Jayce"}
    };
    std::string m2_json = CreateSampleMatchJson("KR_1002", parts2);
    assert(db.SaveMatch("KR_1002", m2_json));
    std::string t2_json = CreateSampleTimelineJson("KR_1002", 4, 5);
    assert(db.SaveTimeline("KR_1002", t2_json));

    assert(db.GetMatchCount() == 2);
    assert(db.GetParticipantCount() == 8);
    assert(db.GetTimelineCount() == 2);
    assert(db.GetTimelineParticipantFrameCount() == 40); // 4 participants * 5 frames * 2 matches
    assert(db.GetTimelineEventCount() == 10); // 5 events * 2 matches
    std::cout << "  [PASS] Seeded database with 2 matches, 8 participants, 40 timeline frames, 10 events." << std::endl;

    // Run test suites
    TestSuite1_SQLSecurity(db);
    TestSuite2_ParserBoundariesAndErrors(db);
    TestSuite3_ComplexQueries(db);
    TestSuite4_CSVExporter(db);
    TestSuite5_SummariesAndTelemetry(db);
    TestSuite6_ConcurrencyStress(db);
    TestSuite7_UninitializedGuards(db);

    // Cleanup
    std::error_code ec;
    std::filesystem::remove_all(TEST_DIR, ec);

    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[SUCCESS] All Milestone 3 Adversarial Test Suites Passed Cleanly!" << std::endl;
    std::cout << "=====================================================================" << std::endl;
    return 0;
}
