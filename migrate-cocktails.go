package main

import (
	"os"
	"strings"
	"path/filepath"
	"encoding/json"
	"io/ioutil"
	"database/sql"

	_ "github.com/mattn/go-sqlite3"
)

type Ingredient struct {
	Name string
	Measure string
	Unit string
}

type Drink struct {
	Name string
	Img []string
	Serve string
	Garnish string
	Drinkware string
	Ingredients []Ingredient
	Method string
}

func main() {
	db, err := sql.Open("sqlite3", "db/db.db")
	if err != nil {
		println("DB error")
		println(err.Error())
		os.Exit(1)
	}

	err = filepath.Walk("drinks", func(path string, info os.FileInfo, err error) error {
		if path == "drinks" {
			return nil // continue
		}

		file, err := ioutil.ReadFile(path)
		if err != nil {
			return err
		}

		var d Drink
		err = json.Unmarshal(file, &d)
		if err != nil {
			return err
		}

		var img string
		if len(d.Img) > 0 {
			img = d.Img[0]
		}
		res, err := db.Exec("INSERT INTO cocktails (title, slug, image, serve, garnish, drinkware, method) VALUES (?,?,?,?,?,?,?)",
			d.Name,
			strings.ReplaceAll(d.Name, " ", "+"),
			img,
			d.Serve,
			d.Garnish,
			d.Drinkware,
			d.Method,
		)
		if err != nil {
			return err
		}

		id, err := res.LastInsertId()
		if err != nil {
			return err
		}

		for _, ingredient := range d.Ingredients {
			_, err := db.Exec("INSERT INTO cocktail_ingredients (name, measure, unit, cocktail_id) VALUES (?,?,?,?)",
				ingredient.Name,
				ingredient.Measure,
				ingredient.Unit,
				id,
			)
			if err != nil {
				return err
			}
		}

		println(d.Name)
		return nil
	})
	if err != nil {
		println("Errored hard bro")
		println(err.Error())
	}
}
