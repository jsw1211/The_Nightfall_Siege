import unreal

mapping_context = unreal.load_asset('/Game/BP/IMC_BaseCharacter')
for mapping in mapping_context.get_editor_property('mappings'):
    action = mapping.get_editor_property('action')
    key = mapping.get_editor_property('key')
    print(f'INPUT_MAPPING action={action.get_name() if action else "None"} key={key.get_editor_property("key_name")}')
