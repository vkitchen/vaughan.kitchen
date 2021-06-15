include(`tmpl/partials/doc_open.m4')
<br><br>
<form action="/login" method="post">
	<fieldset>
		<legend>Login:</legend>
		<label>
			Username:
			<input type="text" name="username">
		</label>
		<br><br>
		<label>
			Password:
			<input type="password" name="password">
		</label>
		<br><br>
		<button>
			Submit
		</button>
	</fieldset>
</form>
include(`tmpl/partials/doc_close.m4')
