#include "Picotado/PluginEditor.h"

#include <unordered_map>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <WebViewFiles.h>

#include "Picotado/ParameterIDs.h"

namespace picotado {

namespace {

std::vector<std::byte> streamToVector(juce::InputStream& stream) {
  const auto sizeInBytes = static_cast<size_t>(stream.getTotalLength());
  std::vector<std::byte> result(sizeInBytes);
  stream.setPosition(0);
  [[maybe_unused]] const auto bytesRead =
      stream.read(result.data(), result.size());
  jassert(static_cast<size_t>(bytesRead) == sizeInBytes);
  return result;
}

const char* getMimeForExtension(const juce::String& extension) {
  static const std::unordered_map<juce::String, const char*> mimeMap = {
      {{"htm"}, "text/html"},   {{"html"}, "text/html"},
      {{"txt"}, "text/plain"},  {{"jpg"}, "image/jpeg"},
      {{"jpeg"}, "image/jpeg"}, {{"svg"}, "image/svg+xml"},
      {{"ico"}, "image/vnd.microsoft.icon"},
      {{"json"}, "application/json"},
      {{"png"}, "image/png"},   {{"css"}, "text/css"},
      {{"map"}, "application/json"},
      {{"js"}, "text/javascript"},
      {{"woff2"}, "font/woff2"}};
  if (auto it = mimeMap.find(extension.toLowerCase()); it != mimeMap.end()) {
    return it->second;
  }
  return "application/octet-stream";
}

#ifndef ZIPPED_FILES_PREFIX
#error \
    "ZIPPED_FILES_PREFIX must be defined (set by plugin/CMakeLists.txt)"
#endif

std::vector<std::byte> getWebViewFileAsBytes(const juce::String& filepath) {
  juce::MemoryInputStream zipStream{webview_files::webview_files_zip,
                                    webview_files::webview_files_zipSize,
                                    false};
  juce::ZipFile zipFile{zipStream};
  if (auto* zipEntry = zipFile.getEntry(ZIPPED_FILES_PREFIX + filepath)) {
    const std::unique_ptr<juce::InputStream> entryStream{
        zipFile.createStreamForEntry(*zipEntry)};
    if (entryStream == nullptr) {
      jassertfalse;
      return {};
    }
    return streamToVector(*entryStream);
  }
  return {};
}

}  // namespace

PicotadoAudioProcessorEditor::PicotadoAudioProcessorEditor(
    PicotadoAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef_(p),
      gainRelay_(id::GAIN.getParamID()),
      dryWetRelay_(id::DRY_WET.getParamID()),
      grainSizeRelay_(id::GRAIN_SIZE.getParamID()),
      densityRelay_(id::DENSITY.getParamID()),
      pitchRelay_(id::PITCH.getParamID()),
      pitchRandRelay_(id::PITCH_RAND.getParamID()),
      positionRelay_(id::POSITION.getParamID()),
      positionRandRelay_(id::POSITION_RAND.getParamID()),
      spreadRelay_(id::SPREAD.getParamID()),
      freezeRelay_(id::FREEZE.getParamID()),
      bypassRelay_(id::BYPASS.getParamID()),
      webView_{
          juce::WebBrowserComponent::Options{}
              .withBackend(
                  juce::WebBrowserComponent::Options::Backend::webview2)
              .withWinWebView2Options(
                  juce::WebBrowserComponent::Options::WinWebView2{}
                      .withBackgroundColour(juce::Colour::fromRGB(18, 18, 24))
                      .withUserDataFolder(juce::File::getSpecialLocation(
                          juce::File::SpecialLocationType::tempDirectory)))
              .withNativeIntegrationEnabled()
              .withResourceProvider(
                  [this](const auto& url) { return getResource(url); })
              .withInitialisationData("vendor", JucePlugin_Manufacturer)
              .withInitialisationData("pluginName", JucePlugin_Name)
              .withInitialisationData("pluginVersion", JucePlugin_VersionString)
              .withNativeFunction(
                  juce::Identifier{"loadSofa"},
                  [this](const juce::Array<juce::var>& args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    loadSofaFunction(args, std::move(completion));
                  })
              .withNativeFunction(
                  juce::Identifier{"unloadSofa"},
                  [this](const juce::Array<juce::var>& args,
                         juce::WebBrowserComponent::NativeFunctionCompletion
                             completion) {
                    unloadSofaFunction(args, std::move(completion));
                  })
              .withOptionsFrom(gainRelay_)
              .withOptionsFrom(dryWetRelay_)
              .withOptionsFrom(grainSizeRelay_)
              .withOptionsFrom(densityRelay_)
              .withOptionsFrom(pitchRelay_)
              .withOptionsFrom(pitchRandRelay_)
              .withOptionsFrom(positionRelay_)
              .withOptionsFrom(positionRandRelay_)
              .withOptionsFrom(spreadRelay_)
              .withOptionsFrom(freezeRelay_)
              .withOptionsFrom(bypassRelay_)},
      gainAttachment_(*processorRef_.getState().getParameter(
                          id::GAIN.getParamID()),
                      gainRelay_, nullptr),
      dryWetAttachment_(*processorRef_.getState().getParameter(
                            id::DRY_WET.getParamID()),
                        dryWetRelay_, nullptr),
      grainSizeAttachment_(*processorRef_.getState().getParameter(
                               id::GRAIN_SIZE.getParamID()),
                           grainSizeRelay_, nullptr),
      densityAttachment_(*processorRef_.getState().getParameter(
                             id::DENSITY.getParamID()),
                         densityRelay_, nullptr),
      pitchAttachment_(*processorRef_.getState().getParameter(
                           id::PITCH.getParamID()),
                       pitchRelay_, nullptr),
      pitchRandAttachment_(*processorRef_.getState().getParameter(
                               id::PITCH_RAND.getParamID()),
                           pitchRandRelay_, nullptr),
      positionAttachment_(*processorRef_.getState().getParameter(
                              id::POSITION.getParamID()),
                          positionRelay_, nullptr),
      positionRandAttachment_(*processorRef_.getState().getParameter(
                                  id::POSITION_RAND.getParamID()),
                              positionRandRelay_, nullptr),
      spreadAttachment_(*processorRef_.getState().getParameter(
                            id::SPREAD.getParamID()),
                        spreadRelay_, nullptr),
      freezeAttachment_(*processorRef_.getState().getParameter(
                            id::FREEZE.getParamID()),
                        freezeRelay_, nullptr),
      bypassAttachment_(*processorRef_.getState().getParameter(
                            id::BYPASS.getParamID()),
                        bypassRelay_, nullptr) {
  addAndMakeVisible(webView_);
  webView_.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

  setResizable(true, true);
  setResizeLimits(720, 820, 1800, 1600);
  setSize(960, 1020);
  startTimer(60);
}

PicotadoAudioProcessorEditor::~PicotadoAudioProcessorEditor() = default;

void PicotadoAudioProcessorEditor::resized() {
  webView_.setBounds(getLocalBounds());
}

void PicotadoAudioProcessorEditor::timerCallback() {
  webView_.emitEventIfBrowserIsVisible("grainStats", juce::var{});
}

auto PicotadoAudioProcessorEditor::getResource(const juce::String& url) const
    -> std::optional<Resource> {
  const auto resourceToRetrieve =
      url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);

  if (resourceToRetrieve == "grainStats.json") {
    juce::DynamicObject::Ptr obj{new juce::DynamicObject{}};
    obj->setProperty("activeGrains", processorRef_.activeGrains.load());
    obj->setProperty("hrtfLoaded",
                     processorRef_.getBinauralRenderer().hasHRTF());
    obj->setProperty(
        "filterLength",
        processorRef_.getBinauralRenderer().getFilterLength());
    obj->setProperty(
        "sofaName",
        processorRef_.getBinauralRenderer().getLoadedFileName());
    const auto json = juce::JSON::toString(obj.get());
    juce::MemoryInputStream stream{json.getCharPointer(),
                                   json.getNumBytesAsUTF8(), false};
    return Resource{streamToVector(stream),
                    juce::String{"application/json"}};
  }

  const auto resource = getWebViewFileAsBytes(resourceToRetrieve);
  if (!resource.empty()) {
    const auto extension =
        resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
    return Resource{std::move(resource), getMimeForExtension(extension)};
  }
  return std::nullopt;
}

void PicotadoAudioProcessorEditor::loadSofaFunction(
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion) {
  juce::ignoreUnused(args);
  // Default the picker to %APPDATA%\Picotado\SOFA — that's where
  // scripts\download-sofa.cmd drops MIT KEMAR. ~/Documents is avoided
  // because Defender's Controlled Folder Access often blocks writes.
  auto sofaFolder = juce::File::getSpecialLocation(
                        juce::File::userApplicationDataDirectory)
                        .getChildFile("Picotado")
                        .getChildFile("SOFA");
  fileChooser_ = std::make_unique<juce::FileChooser>(
      "Select a SOFA HRTF file",
      sofaFolder.isDirectory() ? sofaFolder : juce::File{}, "*.sofa");
  auto callback =
      [this, completion = std::move(completion)](
          const juce::FileChooser& fc) mutable {
        const auto file = fc.getResult();
        if (file == juce::File{}) {
          completion(juce::var{"cancelled"});
          return;
        }
        const bool ok =
            processorRef_.getBinauralRenderer().loadSOFA(file);
        completion(juce::var{ok ? juce::String{"ok"}
                                : juce::String{"failed"}});
      };
  fileChooser_->launchAsync(juce::FileBrowserComponent::openMode |
                                juce::FileBrowserComponent::canSelectFiles,
                            std::move(callback));
}

void PicotadoAudioProcessorEditor::unloadSofaFunction(
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion completion) {
  juce::ignoreUnused(args);
  processorRef_.getBinauralRenderer().unloadSOFA();
  completion(juce::var{"ok"});
}

}  // namespace picotado
