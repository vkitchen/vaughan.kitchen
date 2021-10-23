include(`tmpl/partials/doc_open.m4')
<main>
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
		<label>
			Published:
			<input type="checkbox" name="published" @@published@@>
		</label>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>
include(`tmpl/partials/doc_close.m4')
