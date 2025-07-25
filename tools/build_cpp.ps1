param (
   [string]$relativeFileDirname
)
if ($relativeFileDirname -match "^\d{4}[\\/]Task-\d{1,2}$") {
   $matches = $relativeFileDirname -split '\D+'
   $year = $matches[0]
   $taskNumber = $matches[1]
   $targetName = "aoc_${year}_${taskNumber}"
   cmake --build build --target $targetName
}
