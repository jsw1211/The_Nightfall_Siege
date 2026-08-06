import unreal

ASSET_PATH = "/Game/BP_Character/WBP_Quest"

if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
    unreal.log("WBP_Quest already exists; leaving the existing asset unchanged.")
else:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.QuestWidget)
    widget_bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "WBP_Quest", "/Game/BP_Character", unreal.WidgetBlueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(widget_bp)
    unreal.log("Created /Game/BP_Character/WBP_Quest")
