# ==========================================
# CONFIGURATION
# ==========================================

$SourceFile = "C:\vscode\ESP32-Ampel\db\sensordaten.db"
$BackupFile = "C:\vscode\ESP32-Ampel\BackupService\Backup\sensordaten_backup.db"

$SuccessLog = "C:\vscode\ESP32-Ampel\BackupService\Logs\Success\success.log"
$ErrorLog   = "C:\vscode\ESP32-Ampel\BackupService\Logs\Error\error.log"

$Interval = 60


# ==========================================
# INITIALIZATION
# ==========================================

$Directories = @(
    (Split-Path $BackupFile),
    (Split-Path $SuccessLog),
    (Split-Path $ErrorLog)
)

foreach ($Directory in $Directories) {
    if (!(Test-Path $Directory)) {
        New-Item -ItemType Directory -Path $Directory -Force | Out-Null
    }
}


# ==========================================
# BACKUP LOOP
# ==========================================

Write-Host "Backup service started."

while ($true) {

    $Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    $Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $TempFile = "$BackupFile.tmp"

    try {

        # Check if source file exists
        if (!(Test-Path $SourceFile -PathType Leaf)) {
            throw "Source file does not exist: $SourceFile"
        }

        # Get source file information
        $FileInfo = Get-Item $SourceFile
        $FileSize = $FileInfo.Length

        # Copy to temporary file first
        Copy-Item -Path $SourceFile -Destination $TempFile -Force -ErrorAction Stop

        # Verify copied file size
        $CopiedSize = (Get-Item $TempFile).Length

        if ($CopiedSize -ne $FileSize) {
            throw "File size mismatch! Expected: $FileSize bytes, copied: $CopiedSize bytes."
        }

        # Replace existing backup
        Move-Item -Path $TempFile -Destination $BackupFile -Force -ErrorAction Stop

        # Format file size
        $SizeKB = [math]::Round($CopiedSize / 1KB, 2)
        $SizeMB = [math]::Round($CopiedSize / 1MB, 2)

        # SUCCESS LOG
        $LogEntry = "$Timestamp | SUCCESS | File: $(Split-Path $SourceFile -Leaf) | Written: $CopiedSize Bytes ($SizeKB KB / $SizeMB MB)"

        Add-Content -Path $SuccessLog -Value $LogEntry -Encoding UTF8

        Write-Host $LogEntry -ForegroundColor Green

    }
    catch {

        # ERROR LOG
        $ErrorMessage = $_.Exception.Message

        $LogEntry = "$Timestamp | ERROR | File: $SourceFile | Message: $ErrorMessage"

        Add-Content -Path $ErrorLog -Value $LogEntry -Encoding UTF8

        Write-Host $LogEntry -ForegroundColor Red

        # Remove temporary file if it exists
        if (Test-Path $TempFile) {
            Remove-Item $TempFile -Force -ErrorAction SilentlyContinue
        }
    }

    # Wait until next minute
    $Stopwatch.Stop()

    $Elapsed = $Stopwatch.Elapsed.TotalSeconds
    $Remaining = $Interval - $Elapsed

    if ($Remaining -gt 0) {
        Start-Sleep -Seconds ([math]::Ceiling($Remaining))
    }
}