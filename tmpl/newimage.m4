include(`tmpl/partials/doc_open.m4')
<br>
<form action="/newimage" method="post" enctype="multipart/form-data">
	<label>
		Title:
		<input type="text" name="title">
	</label>
	<br>
	<label>
		Alt Text:
		<input type="text" name="alt">
	</label>
	<br>
	<label>
		Attribution:
		<input type="text" name="attribution">
	</label>
	<br>
	<label>
		Image:
		<input type="file" name="image">
	</label>
	<br>
	<button>
		Submit
	</button>
</form>
include(`tmpl/partials/doc_close.m4')