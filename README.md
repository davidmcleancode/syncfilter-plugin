# SyncFilter — VST3 build

This folder is a complete JUCE plugin project. You don't need Xcode, Visual
Studio, or JUCE installed on your own machine — GitHub's servers compile it
for you every time you push, and hand you back a finished `.vst3` file to
download.

## One-time setup

1. **Create a GitHub account** (skip if you already have one): https://github.com/join

2. **Create a new repository**
   - Click the `+` in the top right of github.com → "New repository"
   - Name it something like `syncfilter-plugin`
   - Leave it **Public** (this keeps GitHub Actions free) and don't add a
     README/gitignore (we already have those) → "Create repository"

3. **Upload this project**
   On the empty repo page, click "uploading an existing file" and drag in
   everything from this folder (keeping the folder structure — `Source/`,
   `.github/`, `CMakeLists.txt`, `.gitignore`). Commit directly to `main`.

   (If you're comfortable with the command line instead:
   ```
   cd SyncFilterPlugin
   git init
   git add .
   git commit -m "Initial plugin"
   git branch -M main
   git remote add origin https://github.com/YOUR-USERNAME/syncfilter-plugin.git
   git push -u origin main
   ```)

## Building

Pushing to `main` automatically triggers the build. To watch it:

1. Go to the **Actions** tab on your repo
2. Click the running workflow ("Build VST3")
3. Wait for the macOS job to finish (a few minutes — it's compiling JUCE
   from scratch the first time)
4. Once it's green, scroll down to **Artifacts** and download
   `SyncFilter-macOS-VST3`

## Installing into Reason

1. Unzip the downloaded artifact — you'll get `SyncFilter.vst3`
2. Move it to `~/Library/Audio/Plug-Ins/VST3/`
3. In Reason, rescan plugins (Preferences → Advanced → Audio Plugins → Rescan)
4. SyncFilter should now show up in your effects list

## Making changes

Every time you (or I) edit the source files and push again, a fresh
`.vst3` shows up in Actions a few minutes later. That's the whole loop —
no local build tools needed.

## What's in here

- `Source/PluginProcessor.*` — the real-time audio engine: host-tempo sync,
  the modulation lookup table, and the filter
- `Source/CurveEditor.*` — the node/breakpoint editor component
- `Source/PluginEditor.*` — the plugin window layout
- `Source/RotaryLookAndFeel.h` — the knob styling (7 o'clock to 5 o'clock sweep)
- `CMakeLists.txt` — build configuration (fetches JUCE automatically)
- `.github/workflows/build.yml` — the GitHub Actions build recipe
