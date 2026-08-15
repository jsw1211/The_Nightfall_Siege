import unreal

task = unreal.AssetImportTask()
task.filename = r'C:/UnrealEngine/The_Nightfall_Siege/Content/BP_Character/Textures/T_DarknessVignette.png'
task.destination_path = '/Game/BP_Character/Textures'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

if not task.imported_object_paths:
    raise RuntimeError('T_DarknessVignette reimport failed')
