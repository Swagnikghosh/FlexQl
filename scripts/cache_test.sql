-- ============================================================
-- FlexQL Cache Test — school database
-- Tables: d2_table1, d3_table1, d_table, grades
-- Schema:
--   d2_table1 / d3_table1 / d_table : ID INT PK, C1-C5 VARCHAR
--   grades                           : STUDENT_ID INT PK, GRADE VARCHAR
--
-- Expected server log patterns (watch with: tail -f server.log):
--   CACHE MISS [raw]    — new SQL string, needs full parse + execute
--   CACHE MISS [page]   — table pages not yet in memory, disk read
--   CACHE HIT  [page]   — table already paged, no disk I/O
--   CACHE HIT  [raw]    — exact same SQL seen before, skip everything
--   CACHE MISS [select] — query result not cached yet
--   CACHE HIT  [select] — query result returned from SLRU cache
--   CACHE INVALIDATE    — write (INSERT/DROP) flushed stale entries
-- ============================================================

USE school;

-- ============================================================
-- 1. SHOW commands — no cache involved
-- ============================================================
SHOW DATABASES;
SHOW TABLES FROM school;

-- ============================================================
-- 2. First SELECT — cold start
--    Expect: MISS [raw] → MISS [page] → MISS [select]
--    Pages for d2_table1 are loaded from disk and stored.
-- ============================================================
SELECT * FROM d2_table1;

-- ============================================================
-- 3. Same SQL again — raw cache hit, skip everything
--    Expect: HIT [raw]
-- ============================================================
SELECT * FROM d2_table1;

-- ============================================================
-- 4. Different columns, same table
--    Expect: MISS [raw] → HIT [page] (spatial: rows already paged)
--            → MISS [select] (this projection not cached yet)
-- ============================================================
SELECT ID, C1 FROM d2_table1;

-- ============================================================
-- 5. Same projection again
--    Expect: HIT [raw]
-- ============================================================
SELECT ID, C1 FROM d2_table1;

-- ============================================================
-- 6. WHERE on a non-PK column
--    Expect: MISS [raw] → HIT [page] → MISS [select]
-- ============================================================
SELECT * FROM d2_table1 WHERE C1 = 'r000005_c1';

-- ============================================================
-- 7. Same WHERE again — temporal: promoted to SLRU protected zone
--    Expect: HIT [raw]
-- ============================================================
SELECT * FROM d2_table1 WHERE C1 = 'r000005_c1';

-- ============================================================
-- 8. Primary-key lookup — bypasses page cache, direct index read
--    Expect: MISS [raw] → (PK path, no page cache) → MISS [select]
-- ============================================================
SELECT * FROM d2_table1 WHERE ID = 3;

-- ============================================================
-- 9. Same PK lookup again
--    Expect: HIT [raw]
-- ============================================================
SELECT * FROM d2_table1 WHERE ID = 3;

-- ============================================================
-- 10. First SELECT on a different table — d3_table1 cold
--     Expect: MISS [raw] → MISS [page] → MISS [select]
-- ============================================================
SELECT * FROM d3_table1;

-- ============================================================
-- 11. Cross-table JOIN — d2_table1 pages already warm
--     Expect: MISS [raw]
--             HIT  [page] for d2_table1  (already loaded in step 2)
--             MISS [page] for grades     (first access)
--             MISS [join]
-- ============================================================
SELECT d2_table1.ID, d2_table1.C1, grades.GRADE
FROM d2_table1
INNER JOIN grades ON d2_table1.ID = grades.STUDENT_ID;

-- ============================================================
-- 12. Same JOIN again — everything warm
--     Expect: HIT [raw]
-- ============================================================
SELECT d2_table1.ID, d2_table1.C1, grades.GRADE
FROM d2_table1
INNER JOIN grades ON d2_table1.ID = grades.STUDENT_ID;

-- ============================================================
-- 13. JOIN with WHERE filter
--     Expect: MISS [raw] → HIT [page] both tables → MISS [join]
-- ============================================================
SELECT d2_table1.ID, d2_table1.C1, grades.GRADE
FROM d2_table1
INNER JOIN grades ON d2_table1.ID = grades.STUDENT_ID
WHERE grades.GRADE = 'A';

-- ============================================================
-- 14. INSERT into d2_table1 — invalidates query cache + page cache
--     Expect: CACHE INVALIDATE [query] school.d2_table1
--             CACHE INVALIDATE [page]  school.d2_table1
-- ============================================================
INSERT INTO d2_table1 VALUES (999, 'test_c1', 'test_c2', 'test_c3', 'test_c4', 'test_c5');

-- ============================================================
-- 15. SELECT after INSERT — cache is cold again for d2_table1
--     Expect: MISS [raw] → MISS [page] → MISS [select]
--     New row (999) must appear in results.
-- ============================================================
SELECT * FROM d2_table1 WHERE ID = 999;

-- ============================================================
-- 16. Same SELECT again — back to warm
--     Expect: HIT [raw]
-- ============================================================
SELECT * FROM d2_table1 WHERE ID = 999;

-- ============================================================
-- 17. Full scan after INSERT — page cache reloaded with new row
--     Expect: MISS [raw] → HIT [page] (reloaded in step 15)
--             → MISS [select]
-- ============================================================
SELECT * FROM d2_table1;

-- ============================================================
-- 18. INSERT into grades — invalidates grades cache only
--     d2_table1 cache must be untouched.
--     Expect: CACHE INVALIDATE [query] school.grades
--             CACHE INVALIDATE [page]  school.grades
-- ============================================================
INSERT INTO grades VALUES (999, 'A+');

-- ============================================================
-- 19. SELECT grades — cold again after invalidation
--     Expect: MISS [raw] → MISS [page] → MISS [select]
-- ============================================================
SELECT * FROM grades WHERE STUDENT_ID = 999;

-- ============================================================
-- 20. SELECT d2_table1 — unaffected by grades invalidation
--     Expect: HIT [raw]  (cached from step 17)
-- ============================================================
SELECT * FROM d2_table1;

-- ============================================================
-- 21. Cleanup — remove test rows
--     Each INSERT triggers invalidation for its table.
-- ============================================================
DROP TABLE IF EXISTS cache_test_tmp;
CREATE TABLE cache_test_tmp (ID INT PRIMARY KEY, NOTE VARCHAR);
INSERT INTO cache_test_tmp VALUES (1, 'hello');
SELECT * FROM cache_test_tmp;
SELECT * FROM cache_test_tmp;
DROP TABLE IF EXISTS cache_test_tmp;
