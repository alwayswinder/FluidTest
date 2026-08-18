import unreal
bp = unreal.load_object(name="/Game/_MyTest/Fluid/Bp/NinjaLive", outer=None)
if bp:
    print("BP:", bp.get_name())
    scs = bp.get_editor_property("SimpleConstructionScript")
    if scs:
        print("SCS:", scs.get_name())
        nodes = scs.get_all_nodes()
        print("Nodes:", len(nodes))
        for n in nodes:
            print(" ", n.get_name(), "-", n.get_class().get_name())
            comp = n.get_editor_property("ComponentTemplate")
            if comp:
                print("   Component:", comp.get_name(), "-", comp.get_class().get_name())
    else:
        print("No SCS")
