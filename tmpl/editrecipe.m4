include(`tmpl/partials/doc_open.m4')
<br>
<form action="@@edit-recipe-path@@" method="post">
	<label>
		Title:
		<input type="text" name="title" value="@@title@@">
	</label>
	<br>
	<label>
		Snippet:
		<textarea name="snippet">@@snippet@@</textarea>
	</label>
	<br>
	<label>
		Content:
		<textarea name="content">@@content@@</textarea>
	</label>
	<br>
	<button>
		Submit
	</button>
</form>
include(`tmpl/partials/doc_close.m4')
