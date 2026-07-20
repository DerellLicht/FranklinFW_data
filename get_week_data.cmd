set ICLOUD_DRV="D:\images\iCloud Photos\iCloudDrive\Downloads\Images"

@if /I "%~1"=="list" goto :list

copy %ICLOUD_DRV%\data*week.csv data
del %ICLOUD_DRV%\data*week.csv
   @goto :eof

:list
c:\utility\ndir64 %ICLOUD_DRV% -1
   @goto :eof
