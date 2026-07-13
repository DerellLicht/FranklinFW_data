set ICLOUD_DRV="D:\images\iCloud Photos\iCloudDrive\Downloads\Images"

copy %ICLOUD_DRV%\data*week.csv data
del %ICLOUD_DRV%\data*week.csv
