BEGIN TRANSACTION;
CREATE TABLE images
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, alt TEXT
	, attribution TEXT
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, hash TEXT NOT NULL UNIQUE -- md5
	, format TEXT NOT NULL -- 'jpg', 'png', 'gif'
	);
ALTER TABLE posts ADD COLUMN image_id INTEGER REFERENCES images(id);
ALTER TABLE recipes ADD COLUMN image_id INTEGER REFERENCES images(id);
COMMIT;
