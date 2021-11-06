BEGIN TRANSACTION;
ALTER TABLE posts ADD COLUMN category TEXT;
UPDATE posts SET category = 'blog';
INSERT INTO posts (title, slug, snippet, ctime, mtime, content, user_id, image_id, published, category)
SELECT title, slug, snippet, ctime, mtime, content, user_id, image_id, published, 'recipes' FROM recipes;
DROP TABLE recipes;
INSERT INTO posts (title, slug, ctime, mtime, content, user_id, published, category)
SELECT title, 'cv', mtime, mtime, content, 1, true, 'pages' FROM pages;
DROP TABLE pages;
COMMIT;
