BEGIN TRANSACTION;
CREATE TABLE pages
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL UNIQUE
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, content TEXT NOT NULL
	);
INSERT INTO pages (title, mtime, content) SELECT 'cv', mtime, content FROM cv WHERE id = 1;
DROP TABLE cv;
COMMIT;
