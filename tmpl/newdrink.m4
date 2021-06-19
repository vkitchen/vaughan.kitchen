include(`tmpl/partials/doc_open.m4')
<main>
	<br>
	<form action="/cocktails/drinks/new" method="post">
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
			Serve:
			<input type="text" name="serve">
		</label>
		<br>
		<label>
			Garnish:
			<input type="text" name="garnish">
		</label>
		<br>
		<label>
			Drinkware:
			<input type="text" name="drinkware">
		</label>
		<br>
		<label>
			Method:
			<textarea name="method"></textarea>
		</label>
		<br>
		<h4>Ingredients:</h4>
		<table>
			<tr><th>#</th><th>Name</th><th>Measure</th><th>Unit</th></tr>
			<tr>
				<td>1</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>2</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>3</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>4</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>5</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>6</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>7</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>8</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>9</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
			<tr>
				<td>10</td>
				<td><input type="text" name="ingredient_name"></td>
				<td><input type="number" name="ingredient_measure"></td>
				<td><select name="ingredient_unit">
					<option value="barspoon">barspoon</option>
					<option value="bottle">bottle</option>
					<option value="can">can</option>
					<option value="cl">cl</option>
					<option value="cube">cube</option>
					<option value="cup">cup</option>
					<option value="dash">dash</option>
					<option value="dashes">dashes</option>
					<option value="drop">drop</option>
					<option value="drops">drops</option>
					<option value="litre">litre</option>
					<option value="ml">ml</option>
					<option selected="selected" value="none">none</option>
					<option value="oz">oz</option>
					<option value="part">part</option>
					<option value="pinch">pinch</option>
					<option value="pint">pint</option>
					<option value="shot">shot</option>
					<option value="splash">splash</option>
					<option value="spoon">spoon</option>
					<option value="sprig">sprig</option>
					<option value="taste">taste</option>
					<option value="tbsp">Tbsp</option>
					<option value="top">top</option>
					<option value="tsp">tsp</option>
					<option value="wedges">wedges</option>
				</select></td>
			</tr>
		</table>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>
include(`tmpl/partials/doc_close.m4')
