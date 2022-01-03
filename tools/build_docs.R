.libPaths(c("C:/Users/thothal/Documents/6. R/win-library/4.1",
				"C:/Users/tthaler/Documents/R/win-library/4.1"))
pan_loc <- rmarkdown::find_pandoc(dir = "c:/Program Files/RStudio/bin/pandoc/")

suppressWarnings(
	suppressPackageStartupMessages({
		library(here)
		library(glue)
		library(purrr)
		library(stringr)
		library(dplyr)
		library(cli)
	})
)
force_year <- "2021"

edit_file <- function(lines, file, replace, start, idx = NULL) {
	if (replace) {
		if (is.null(idx)) {
			stopifnot(length(start) == 1)
			## consecutive lines
			end <- start + length(lines) - 1
			stopifnot(end <= length(file))
			idx <- seq(start, end, 1)
		}
		stopifnot(length(idx) == length(lines))
		file[idx] <- lines
	} else {
		if (start > length(file)) {
			file <- c(file, lines)
		} else {
			file <- c(file[seq_len(start - 1)], lines, file[seq(start, length(file), 1)])
		}
	}
	file
}

year <- coalesce(force_year, format(Sys.Date(), "%Y"))
all_solutions <- list.files(here("docs"), pattern = glue("^{year}_.*\\.html")) %>% 
	str_sort(numeric = TRUE)

## README.md
cli_alert_info("Updating README.md")

readme <- readLines(here("README.md"))

## check if tasks for the year are already present => add if not
if (!any(str_detect(readme, glue("^### *{year}")))) {
	new_lines <- c(glue("### {year}"), "", 
						glue("- [ ] Day {1:25}"), 
						"")
	readme <- edit_file(new_lines, readme, FALSE, str_which(readme, "## ToC") + 2)
}

## find the line numbers for the current year
sec_markers <- str_which(readme, "#+")
year_start <- str_which(readme, glue("### *{year}"))
year_end <- sec_markers[sec_markers > year_start][1]
stopifnot(length(year_start) && !is.na(year_start) &&
			 	length(year_end) && !is.na(year_end))


replacements <- glue("- [x] [Day {day}]({url})",
							day = str_extract(all_solutions, "task\\d+") %>% 
								str_remove("task"),
							url = glue("https://thothal.github.io/AoC/{all_solutions}"))

## find the corresponding tasks within this year

idx <- map_int(str_extract(replacements, "Day \\d+"), function(day) {
	which(str_detect(readme, glue("{day}\\b")) & 
				between(seq_along(readme), year_start, year_end))
})

## replace
readme <- edit_file(replacements, readme, TRUE, idx = idx)
cat(readme, file = here("README.md"), sep = "\n")

## index.Rmd
cli_alert_info("Updating index.Rmd")

index <- readLines(here("index.Rmd"))

## check if the year is already present => add if not
if (!any(str_detect(index, glue("^# *{year}")))) {
	new_line <- c(glue("# {year}"), "")
	index <- edit_file(new_line, index, FALSE, 
							 max(str_which(index, fixed("```"))) + 2)
}

## find the line numbers for the current year
sec_markers <- str_which(index, "#+")
year_start <- str_which(index, glue("# *{year}")) + 1
year_end <- sec_markers[sec_markers > year_start][1] - 1
if (is.na(year_end)) {
	year_end <- length(index)
}

## remove the whole year
index <- index[-seq(year_start, year_end, 1)]

replacements <- glue("* [Day {day}]({url})",
							day = str_extract(all_solutions, "task\\d+") %>% 
								str_remove("task") %>% 
								str_sort(numeric = TRUE),
							url = glue("{all_solutions}"))

## replace
index <- edit_file(c("", replacements, ""), index, FALSE, 
						 str_which(index, glue("# *{year}")) + 1)

cat(index, file = here("index.Rmd"), sep = "\n")

## render Rmd
cli_alert_info("Rendering index.Rmd")
rmarkdown::render(here::here("index.Rmd"), 
						output_file = here::here("docs", "index"))

cli_alert_success("Updating docs done")
q(status = 0)
