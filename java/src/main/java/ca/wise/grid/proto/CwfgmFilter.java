// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: cwfgmFilter.proto

package ca.wise.grid.proto;

public final class CwfgmFilter {
  private CwfgmFilter() {}
  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistryLite registry) {
  }

  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistry registry) {
    registerAllExtensions(
        (com.google.protobuf.ExtensionRegistryLite) registry);
  }
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmAttributeFilter_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmAttributeFilter_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmAttributeFilter_File_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmAttributeFilter_File_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmAttributeFilter_BinaryData_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmAttributeFilter_BinaryData_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmReplaceGridFilterBase_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmReplaceGridFilterBase_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmReplaceGridFilter_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmReplaceGridFilter_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmPolyReplaceGridFilter_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmPolyReplaceGridFilter_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmVectorFilter_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmVectorFilter_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmAsset_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmAsset_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmTarget_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmTarget_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_TemporalCondition_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_TemporalCondition_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_TemporalCondition_DailyAttribute_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_TemporalCondition_DailyAttribute_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_EffectiveAttribute_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_EffectiveAttribute_fieldAccessorTable;

  public static com.google.protobuf.Descriptors.FileDescriptor
      getDescriptor() {
    return descriptor;
  }
  private static  com.google.protobuf.Descriptors.FileDescriptor
      descriptor;
  static {
    java.lang.String[] descriptorData = {
      "\n\021cwfgmFilter.proto\022\016WISE.GridProto\032\nmat" +
      "h.proto\032\013wtime.proto\032\017geography.proto\032\036g" +
      "oogle/protobuf/wrappers.proto\"\336\010\n\024CwfgmA" +
      "ttributeFilter\022\017\n\007version\030\001 \001(\005\0229\n\004file\030" +
      "\002 \001(\0132).WISE.GridProto.CwfgmAttributeFil" +
      "ter.FileH\000\022A\n\006binary\030\003 \001(\0132/.WISE.GridPr" +
      "oto.CwfgmAttributeFilter.BinaryDataH\000\022\021\n" +
      "\004name\030\004 \001(\tH\001\210\001\001\022\025\n\010comments\030\005 \001(\tH\002\210\001\001\032" +
      "k\n\004File\022\020\n\010filename\030\001 \001(\t\022\022\n\nprojection\030" +
      "\002 \001(\t\022=\n\007datakey\030\003 \001(\0162,.WISE.GridProto." +
      "CwfgmAttributeFilter.DataKey\032\273\003\n\nBinaryD" +
      "ata\022\r\n\005xSize\030\001 \001(\r\022\r\n\005ySize\030\002 \001(\r\022-\n\003key" +
      "\030\003 \001(\0132\034.google.protobuf.UInt32ValueB\002\030\001" +
      "\0227\n\004type\030\004 \001(\0162).WISE.GridProto.CwfgmAtt" +
      "ributeFilter.Type\022)\n\004data\030\005 \001(\0132\033.google" +
      ".protobuf.BytesValue\022+\n\006nodata\030\006 \001(\0132\033.g" +
      "oogle.protobuf.BytesValue\022\037\n\txLLCorner\030\007" +
      " \001(\0132\014.Math.Double\022\037\n\tyLLCorner\030\010 \001(\0132\014." +
      "Math.Double\022 \n\nresolution\030\t \001(\0132\014.Math.D" +
      "ouble\022,\n\010isZipped\030\n \001(\0132\032.google.protobu" +
      "f.BoolValue\022=\n\007datakey\030\013 \001(\0162,.WISE.Grid" +
      "Proto.CwfgmAttributeFilter.DataKey\"\262\001\n\004T" +
      "ype\022\t\n\005EMPTY\020\000\022\010\n\004BOOL\020\001\022\010\n\004CHAR\020\002\022\t\n\005SH" +
      "ORT\020\003\022\013\n\007INTEGER\020\004\022\010\n\004LONG\020\005\022\021\n\rUNSIGNED" +
      "_CHAR\020\006\022\022\n\016UNSIGNED_SHORT\020\007\022\024\n\020UNSIGNED_" +
      "INTEGER\020\010\022\021\n\rUNSIGNED_LONG\020\t\022\t\n\005FLOAT\020\n\022" +
      "\016\n\nLONG_FLOAT\020\013\"\216\001\n\007DataKey\022\014\n\010DK_EMPTY\020" +
      "\000\022\013\n\007GREENUP\020\004\022\007\n\002PC\020\371y\022\010\n\003PDF\020\372y\022\021\n\014CUR" +
      "INGDEGREE\020\373y\022\010\n\003CBH\020\366y\022\017\n\nTREEHEIGHT\020\370y\022" +
      "\r\n\010FUELLOAD\020\207z\022\014\n\007FBP_OVD\020\265\020\022\n\n\004FUEL\020\377\377\003" +
      "B\006\n\004dataB\007\n\005_nameB\013\n\t_comments\"\270\002\n\032Cwfgm" +
      "ReplaceGridFilterBase\022\017\n\007version\030\001 \001(\005\022\025" +
      "\n\013toFuelIndex\030\002 \001(\005H\000\022\024\n\ntoFuelName\030\003 \001(" +
      "\tH\000\022\027\n\rfromFuelIndex\030\004 \001(\005H\001\022\026\n\014fromFuel" +
      "Name\030\005 \001(\tH\001\022O\n\014fromFuelRule\030\006 \001(\01627.WIS" +
      "E.GridProto.CwfgmReplaceGridFilterBase.F" +
      "romFuelRuleH\001\"D\n\014FromFuelRule\022\n\n\006NODATA\020" +
      "\000\022\r\n\tALL_FUELS\020\001\022\031\n\025ALL_COMBUSTIBLE_FUEL" +
      "S\020\002B\010\n\006toFuelB\n\n\010fromFuel\"\206\003\n\026CwfgmRepla" +
      "ceGridFilter\022\017\n\007version\030\001 \001(\005\022%\n\010pointOn" +
      "e\030\002 \001(\0132\023.Geography.GeoPoint\022%\n\010pointTwo" +
      "\030\003 \001(\0132\023.Geography.GeoPoint\022\037\n\txLLCorner" +
      "\030\004 \001(\0132\014.Math.Double\022\037\n\tyLLCorner\030\005 \001(\0132" +
      "\014.Math.Double\022 \n\nresolution\030\006 \001(\0132\014.Math" +
      ".Double\022-\n\tlandscape\030\007 \001(\0132\032.google.prot" +
      "obuf.BoolValue\022\021\n\004name\030\010 \001(\tH\000\210\001\001\022\025\n\010com" +
      "ments\030\t \001(\tH\001\210\001\001\022:\n\006filter\030\n \001(\0132*.WISE." +
      "GridProto.CwfgmReplaceGridFilterBaseB\007\n\005" +
      "_nameB\013\n\t_comments\"\250\002\n\032CwfgmPolyReplaceG" +
      "ridFilter\022\017\n\007version\030\001 \001(\005\022&\n\010polygons\030\002" +
      " \001(\0132\022.Geography.GeoPolyH\000\022\022\n\010filename\030\003" +
      " \001(\tH\000\022\021\n\004name\030\004 \001(\tH\001\210\001\001\022\025\n\010comments\030\005 " +
      "\001(\tH\002\210\001\001\022\022\n\005color\030\006 \001(\rH\003\210\001\001\022\021\n\004size\030\007 \001" +
      "(\004H\004\210\001\001\022:\n\006filter\030\010 \001(\0132*.WISE.GridProto" +
      ".CwfgmReplaceGridFilterBaseB\007\n\005shapeB\007\n\005" +
      "_nameB\013\n\t_commentsB\010\n\006_colorB\007\n\005_size\"\364\002" +
      "\n\021CwfgmVectorFilter\022\017\n\007version\030\001 \001(\005\022$\n\016" +
      "fireBreakWidth\030\002 \001(\0132\014.Math.Double\022&\n\010po" +
      "lygons\030\003 \001(\0132\022.Geography.GeoPolyH\000\022\022\n\010fi" +
      "lename\030\004 \001(\tH\000\022\021\n\004name\030\005 \001(\tH\001\210\001\001\022\025\n\010com" +
      "ments\030\006 \001(\tH\002\210\001\001\022\022\n\005color\030\007 \001(\rH\003\210\001\001\022\026\n\t" +
      "fillColor\030\010 \001(\rH\004\210\001\001\022\023\n\006symbol\030\t \001(\004H\005\210\001" +
      "\001\022\022\n\005width\030\n \001(\021H\006\210\001\001\022\025\n\010imported\030\013 \001(\010H" +
      "\007\210\001\001B\006\n\004dataB\007\n\005_nameB\013\n\t_commentsB\010\n\006_c" +
      "olorB\014\n\n_fillColorB\t\n\007_symbolB\010\n\006_widthB" +
      "\013\n\t_imported\"\352\002\n\nCwfgmAsset\022\017\n\007version\030\001" +
      " \001(\005\022#\n\rassetBoundary\030\002 \001(\0132\014.Math.Doubl" +
      "e\022$\n\006assets\030\003 \001(\0132\022.Geography.GeoPolyH\000\022" +
      "\022\n\010filename\030\004 \001(\tH\000\022\021\n\004name\030\005 \001(\tH\001\210\001\001\022\025" +
      "\n\010comments\030\006 \001(\tH\002\210\001\001\022\022\n\005color\030\007 \001(\rH\003\210\001" +
      "\001\022\026\n\tfillColor\030\010 \001(\rH\004\210\001\001\022\023\n\006symbol\030\t \001(" +
      "\004H\005\210\001\001\022\022\n\005width\030\n \001(\021H\006\210\001\001\022\025\n\010imported\030\013" +
      " \001(\010H\007\210\001\001B\006\n\004dataB\007\n\005_nameB\013\n\t_commentsB" +
      "\010\n\006_colorB\014\n\n_fillColorB\t\n\007_symbolB\010\n\006_w" +
      "idthB\013\n\t_imported\"\255\002\n\013CwfgmTarget\022\017\n\007ver" +
      "sion\030\001 \001(\005\022%\n\007targets\030\002 \001(\0132\022.Geography." +
      "GeoPolyH\000\022\022\n\010filename\030\003 \001(\tH\000\022\021\n\004name\030\004 " +
      "\001(\tH\001\210\001\001\022\025\n\010comments\030\005 \001(\tH\002\210\001\001\022\022\n\005color" +
      "\030\006 \001(\rH\003\210\001\001\022\023\n\006symbol\030\007 \001(\004H\004\210\001\001\022\030\n\013disp" +
      "laySize\030\010 \001(\021H\005\210\001\001\022\025\n\010imported\030\t \001(\010H\006\210\001" +
      "\001B\006\n\004dataB\007\n\005_nameB\013\n\t_commentsB\010\n\006_colo" +
      "rB\t\n\007_symbolB\016\n\014_displaySizeB\013\n\t_importe" +
      "d\"\215\t\n\021TemporalCondition\022\017\n\007version\030\001 \001(\005" +
      "\022?\n\005daily\030\002 \003(\01320.WISE.GridProto.Tempora" +
      "lCondition.DailyAttribute\022E\n\010seasonal\030\003 " +
      "\003(\01323.WISE.GridProto.TemporalCondition.S" +
      "easonalAttribute\032\321\004\n\016DailyAttribute\022\017\n\007v" +
      "ersion\030\001 \001(\005\022(\n\016localStartTime\030\002 \001(\0132\020.H" +
      "SS.Times.WTime\022\'\n\tstartTime\030\003 \001(\0132\024.HSS." +
      "Times.WTimeSpan\022%\n\007endTime\030\004 \001(\0132\024.HSS.T" +
      "imes.WTimeSpan\022\033\n\005minRh\030\005 \001(\0132\014.Math.Dou" +
      "ble\022\033\n\005maxWs\030\006 \001(\0132\014.Math.Double\022\034\n\006minF" +
      "wi\030\007 \001(\0132\014.Math.Double\022\034\n\006minIsi\030\010 \001(\0132\014" +
      ".Math.Double\022b\n\026localStartTimeRelative\030\t" +
      " \001(\0162=.WISE.GridProto.TemporalCondition." +
      "DailyAttribute.TimeRelativeH\000\210\001\001\022`\n\024loca" +
      "lEndTimeRelative\030\n \001(\0162=.WISE.GridProto." +
      "TemporalCondition.DailyAttribute.TimeRel" +
      "ativeH\001\210\001\001\"D\n\014TimeRelative\022\022\n\016LOCAL_MIDN" +
      "IGHT\020\000\022\016\n\nLOCAL_NOON\020\001\022\020\n\014SUN_RISE_SET\020\002" +
      "B\031\n\027_localStartTimeRelativeB\027\n\025_localEnd" +
      "TimeRelative\032\212\003\n\021SeasonalAttribute\022\017\n\007ve" +
      "rsion\030\001 \001(\005\022,\n\016localStartTime\030\002 \001(\0132\024.HS" +
      "S.Times.WTimeSpan\022Z\n\nattributes\030\003 \003(\0132F." +
      "WISE.GridProto.TemporalCondition.Seasona" +
      "lAttribute.EffectiveAttribute\032\331\001\n\022Effect" +
      "iveAttribute\022Y\n\004type\030\001 \001(\0162K.WISE.GridPr" +
      "oto.TemporalCondition.SeasonalAttribute." +
      "EffectiveAttribute.Type\022\016\n\006active\030\002 \001(\010\022" +
      "\033\n\005value\030\003 \001(\0132\014.Math.Double\";\n\004Type\022\021\n\r" +
      "CURING_DEGREE\020\000\022\023\n\017GRASS_PHENOLOGY\020\001\022\013\n\007" +
      "GREENUP\020\002B\'\n\022ca.wise.grid.protoP\001\252\002\016WISE" +
      ".GridProtob\006proto3"
    };
    descriptor = com.google.protobuf.Descriptors.FileDescriptor
      .internalBuildGeneratedFileFrom(descriptorData,
        new com.google.protobuf.Descriptors.FileDescriptor[] {
          ca.hss.math.proto.Math.getDescriptor(),
          ca.hss.times.proto.WTimePackage.getDescriptor(),
          ca.hss.math.proto.Geography.getDescriptor(),
          com.google.protobuf.WrappersProto.getDescriptor(),
        });
    internal_static_WISE_GridProto_CwfgmAttributeFilter_descriptor =
      getDescriptor().getMessageTypes().get(0);
    internal_static_WISE_GridProto_CwfgmAttributeFilter_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmAttributeFilter_descriptor,
        new java.lang.String[] { "Version", "File", "Binary", "Name", "Comments", "Data", "Name", "Comments", });
    internal_static_WISE_GridProto_CwfgmAttributeFilter_File_descriptor =
      internal_static_WISE_GridProto_CwfgmAttributeFilter_descriptor.getNestedTypes().get(0);
    internal_static_WISE_GridProto_CwfgmAttributeFilter_File_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmAttributeFilter_File_descriptor,
        new java.lang.String[] { "Filename", "Projection", "Datakey", });
    internal_static_WISE_GridProto_CwfgmAttributeFilter_BinaryData_descriptor =
      internal_static_WISE_GridProto_CwfgmAttributeFilter_descriptor.getNestedTypes().get(1);
    internal_static_WISE_GridProto_CwfgmAttributeFilter_BinaryData_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmAttributeFilter_BinaryData_descriptor,
        new java.lang.String[] { "XSize", "YSize", "Key", "Type", "Data", "Nodata", "XLLCorner", "YLLCorner", "Resolution", "IsZipped", "Datakey", });
    internal_static_WISE_GridProto_CwfgmReplaceGridFilterBase_descriptor =
      getDescriptor().getMessageTypes().get(1);
    internal_static_WISE_GridProto_CwfgmReplaceGridFilterBase_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmReplaceGridFilterBase_descriptor,
        new java.lang.String[] { "Version", "ToFuelIndex", "ToFuelName", "FromFuelIndex", "FromFuelName", "FromFuelRule", "ToFuel", "FromFuel", });
    internal_static_WISE_GridProto_CwfgmReplaceGridFilter_descriptor =
      getDescriptor().getMessageTypes().get(2);
    internal_static_WISE_GridProto_CwfgmReplaceGridFilter_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmReplaceGridFilter_descriptor,
        new java.lang.String[] { "Version", "PointOne", "PointTwo", "XLLCorner", "YLLCorner", "Resolution", "Landscape", "Name", "Comments", "Filter", "Name", "Comments", });
    internal_static_WISE_GridProto_CwfgmPolyReplaceGridFilter_descriptor =
      getDescriptor().getMessageTypes().get(3);
    internal_static_WISE_GridProto_CwfgmPolyReplaceGridFilter_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmPolyReplaceGridFilter_descriptor,
        new java.lang.String[] { "Version", "Polygons", "Filename", "Name", "Comments", "Color", "Size", "Filter", "Shape", "Name", "Comments", "Color", "Size", });
    internal_static_WISE_GridProto_CwfgmVectorFilter_descriptor =
      getDescriptor().getMessageTypes().get(4);
    internal_static_WISE_GridProto_CwfgmVectorFilter_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmVectorFilter_descriptor,
        new java.lang.String[] { "Version", "FireBreakWidth", "Polygons", "Filename", "Name", "Comments", "Color", "FillColor", "Symbol", "Width", "Imported", "Data", "Name", "Comments", "Color", "FillColor", "Symbol", "Width", "Imported", });
    internal_static_WISE_GridProto_CwfgmAsset_descriptor =
      getDescriptor().getMessageTypes().get(5);
    internal_static_WISE_GridProto_CwfgmAsset_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmAsset_descriptor,
        new java.lang.String[] { "Version", "AssetBoundary", "Assets", "Filename", "Name", "Comments", "Color", "FillColor", "Symbol", "Width", "Imported", "Data", "Name", "Comments", "Color", "FillColor", "Symbol", "Width", "Imported", });
    internal_static_WISE_GridProto_CwfgmTarget_descriptor =
      getDescriptor().getMessageTypes().get(6);
    internal_static_WISE_GridProto_CwfgmTarget_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmTarget_descriptor,
        new java.lang.String[] { "Version", "Targets", "Filename", "Name", "Comments", "Color", "Symbol", "DisplaySize", "Imported", "Data", "Name", "Comments", "Color", "Symbol", "DisplaySize", "Imported", });
    internal_static_WISE_GridProto_TemporalCondition_descriptor =
      getDescriptor().getMessageTypes().get(7);
    internal_static_WISE_GridProto_TemporalCondition_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_TemporalCondition_descriptor,
        new java.lang.String[] { "Version", "Daily", "Seasonal", });
    internal_static_WISE_GridProto_TemporalCondition_DailyAttribute_descriptor =
      internal_static_WISE_GridProto_TemporalCondition_descriptor.getNestedTypes().get(0);
    internal_static_WISE_GridProto_TemporalCondition_DailyAttribute_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_TemporalCondition_DailyAttribute_descriptor,
        new java.lang.String[] { "Version", "LocalStartTime", "StartTime", "EndTime", "MinRh", "MaxWs", "MinFwi", "MinIsi", "LocalStartTimeRelative", "LocalEndTimeRelative", "LocalStartTimeRelative", "LocalEndTimeRelative", });
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_descriptor =
      internal_static_WISE_GridProto_TemporalCondition_descriptor.getNestedTypes().get(1);
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_descriptor,
        new java.lang.String[] { "Version", "LocalStartTime", "Attributes", });
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_EffectiveAttribute_descriptor =
      internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_descriptor.getNestedTypes().get(0);
    internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_EffectiveAttribute_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_TemporalCondition_SeasonalAttribute_EffectiveAttribute_descriptor,
        new java.lang.String[] { "Type", "Active", "Value", });
    ca.hss.math.proto.Math.getDescriptor();
    ca.hss.times.proto.WTimePackage.getDescriptor();
    ca.hss.math.proto.Geography.getDescriptor();
    com.google.protobuf.WrappersProto.getDescriptor();
  }

  // @@protoc_insertion_point(outer_class_scope)
}