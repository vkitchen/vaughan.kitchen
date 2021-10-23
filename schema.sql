CREATE TABLE users
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, display TEXT NOT NULL -- name for display
	, login TEXT UNIQUE NOT NULL
	, shadow TEXT NOT NULL
	);

CREATE TABLE sessions
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, cookie TEXT UNIQUE NOT NULL
	, user_id INTEGER NOT NULL
	, FOREIGN KEY(user_id) REFERENCES users(id)
	);

CREATE TABLE images
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, alt TEXT
	, attribution TEXT
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, hash TEXT NOT NULL UNIQUE -- md5
	, format TEXT NOT NULL -- 'jpg', 'png', 'gif'
	);

CREATE TABLE pages
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL UNIQUE
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, content TEXT NOT NULL
	);

CREATE TABLE posts
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, slug TEXT NOT NULL UNIQUE
	, snippet TEXT
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, content TEXT NOT NULL
	, user_id INTEGER NOT NULL -- author
	, image_id INTEGER
	, published BOOLEAN NOT NULL DEFAULT(false)
	, FOREIGN KEY(user_id) REFERENCES users(id)
	, FOREIGN KEY(image_id) REFERENCES images(id)
	);

-- Same format as a post (at least for now)
CREATE TABLE recipes
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, slug TEXT NOT NULL UNIQUE
	, snippet TEXT
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, content TEXT NOT NULL
	, user_id INTEGER NOT NULL -- author
	, image_id INTEGER
	, published BOOLEAN NOT NULL DEFAULT(false)
	, FOREIGN KEY(user_id) REFERENCES users(id)
	, FOREIGN KEY(image_id) REFERENCES images(id)
	);

CREATE TABLE cocktails
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

CREATE TABLE cocktail_ingredients
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, name TEXT NOT NULL
	, measure TEXT NOT NULL -- TODO should probably be INTEGER
	, unit TEXT NOT NULL
	, cocktail_id INTEGER NOT NULL
	, FOREIGN KEY(cocktail_id) REFERENCES cocktails(id)
	);
