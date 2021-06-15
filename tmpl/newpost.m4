include(`tmpl/partials/doc_open.m4')
<br>
<form action="/newpost" method="post">
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
include(`tmpl/partials/doc_close.m4')
