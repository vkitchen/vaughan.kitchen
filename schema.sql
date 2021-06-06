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

-- Should only have one row
CREATE TABLE cv
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, content TEXT NOT NULL
	, lock INTEGER NOT NULL UNIQUE DEFAULT(0) -- DO NOT SET THIS VALUE. Enforces single rowness
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
	, FOREIGN KEY(user_id) REFERENCES users(id)
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
	, FOREIGN KEY(user_id) REFERENCES users(id)
	);

CREATE TABLE cocktails
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, title TEXT NOT NULL
	, slug TEXT NOT NULL
	, ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- creation: unix time
	, mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')) -- modified: unix time
	, serve TEXT
	, garnish TEXT
	, drinkware TEXT
	, method TEXT
	);

CREATE TABLE cocktail_ingredients
	( id INTEGER PRIMARY KEY AUTOINCREMENT
	, name TEXT NOT NULL
	, measure TEXT NOT NULL
	, unit TEXT NOT NULL
	, cocktail_id INTEGER NOT NULL
	, FOREIGN KEY(cocktail_id) REFERENCES cocktails(id)
	);
