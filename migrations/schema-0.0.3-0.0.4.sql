BEGIN TRANSACTION;
CREATE TABLE cocktails_new
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, slug TEXT NOT NULL UNIQUE
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, serve TEXT
	, garnish TEXT
	, drinkware TEXT
	, method TEXT
	, image_id INTEGER
	, FOREIGN KEY(image_id) REFERENCES images(id)
	);
INSERT INTO cocktails_new (id, title, slug, ctime, mtime, serve, garnish, drinkware, method)
SELECT id, title, slug, ctime, mtime, serve, garnish, drinkware, method FROM cocktails;
DROP TABLE cocktails;
ALTER TABLE cocktails_new RENAME TO cocktails;
COMMIT;
