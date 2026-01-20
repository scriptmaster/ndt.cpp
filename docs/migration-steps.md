# Manual Migration Steps

## Files Still Need to Be Moved

The Makefile is updated, but files still need physical moves:

## Manual Commands to Run

**In PowerShell (recommended):**
```powershell
# Create directories
New-Item -ItemType Directory -Force -Path "src" | Out-Null
New-Item -ItemType Directory -Force -Path "archive" | Out-Null
New-Item -ItemType Directory -Force -Path "config/scenes" | Out-Null

# Move App and Services
Move-Item -Path "App" -Destination "src/App" -Force
Move-Item -Path "Services" -Destination "src/Services" -Force

# Archive display files
Get-ChildItem -Path "display" -Filter "*.cpp" | ForEach-Object {
    Move-Item -Path $_.FullName -Destination "archive/$($_.Name).old" -Force
}
Get-ChildItem -Path "display" -Filter "*.h" | ForEach-Object {
    Move-Item -Path $_.FullName -Destination "archive/$($_.Name).old" -Force
}

# Move scenes
Move-Item -Path "scenes/*" -Destination "config/scenes/" -Force
Remove-Item -Path "scenes" -Force
```

**Or in Git Bash:**
```bash
mkdir -p src archive config/scenes
mv App src/App
mv Services src/Services
for f in display/*.cpp; do mv "$f" "archive/$(basename "$f").old"; done
for f in display/*.h; do mv "$f" "archive/$(basename "$f").old"; done
mv scenes/* config/scenes/
rmdir scenes
```

## After Files Are Moved

Update includes:
- `src/App/AppHost.cpp`: `"Services/..."` → `"../Services/..."`

Other includes should work with `-Isrc` flag in Makefile.
