include(`tmpl/partials/doc_open.m4')
<main>
	<br>
	<form action="@@edit-image-path@@" method="post">
		<label>
			Title:
			<input type="text" name="title" value="@@title@@">
		</label>
		<br>
		<label>
			Alt Text:
			<input type="text" name="alt" value="@@alt@@">
		</label>
		<br>
		<label>
			Attribution:
			<input type="text" name="attribution" value="@@attribution@@">
		</label>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>
include(`tmpl/partials/doc_close.m4')