include(`tmpl/partials/doc_open.m4')
<main>
	<br>
	<form action="/newrecipe" method="post">
		<label>
			Title:
			<input type="text" name="title">
		</label>
		<br>
		<label>
			Slug:
			<input type="text" name="slug">
		</label>
		<br>
		<label>
			Snippet:
			<textarea name="snippet"></textarea>
		</label>
		<br>
		<label>
			Content:
			<textarea name="content"></textarea>
		</label>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>
include(`tmpl/partials/doc_close.m4')
