using System;
using System.IO;
using System.IO.Compression;
using System.Text;

const int Width = 1280;
const int Height = 720;
var pixels = new byte[Width * Height * 4];
for (var y = 0; y < Height; ++y)
for (var x = 0; x < Width; ++x)
{
    // Use the screen height for both axes: on 1280x720 this stays a circle.
    var dx = (x + 0.5f - Width * 0.5f) / Height;
    var dy = (y + 0.5f - Height * 0.5f) / Height;
    var radius = MathF.Sqrt(dx * dx + dy * dy);
    // Half-sized vertical diameter: clear centre 65px, fully dark by 215px.
    var alpha = Math.Clamp((radius - 0.045f) / 0.105f, 0f, 1f);
    alpha = alpha * alpha * (3f - 2f * alpha);
    var offset = (y * Width + x) * 4;
    pixels[offset + 3] = (byte)(alpha * 235f);
}

using var output = File.Create(args[0]);
output.Write(new byte[] { 137, 80, 78, 71, 13, 10, 26, 10 });
WriteChunk(output, "IHDR", Header());
using var raw = new MemoryStream();
for (var y = 0; y < Height; ++y) { raw.WriteByte(0); raw.Write(pixels, y * Width * 4, Width * 4); }
using var compressed = new MemoryStream();
using (var zlib = new ZLibStream(compressed, CompressionLevel.SmallestSize, true)) raw.WriteTo(zlib);
WriteChunk(output, "IDAT", compressed.ToArray());
WriteChunk(output, "IEND", Array.Empty<byte>());

byte[] Header()
{
    var data = new byte[13];
    WriteBuffer(data, 0, Width); WriteBuffer(data, 4, Height); data[8] = 8; data[9] = 6;
    return data;
}
void WriteBuffer(byte[] data, int offset, int value)
{
    data[offset] = (byte)(value >> 24); data[offset + 1] = (byte)(value >> 16);
    data[offset + 2] = (byte)(value >> 8); data[offset + 3] = (byte)value;
}
void WriteChunk(Stream stream, string name, byte[] data)
{
    var type = Encoding.ASCII.GetBytes(name);
    UInt(stream, (uint)data.Length); stream.Write(type); stream.Write(data);
    var crcInput = new byte[type.Length + data.Length];
    Buffer.BlockCopy(type, 0, crcInput, 0, type.Length); Buffer.BlockCopy(data, 0, crcInput, type.Length, data.Length);
    UInt(stream, Crc(crcInput));
}
void UInt(Stream stream, uint value)
{
    stream.WriteByte((byte)(value >> 24)); stream.WriteByte((byte)(value >> 16));
    stream.WriteByte((byte)(value >> 8)); stream.WriteByte((byte)value);
}
uint Crc(byte[] data)
{
    uint crc = 0xffffffff;
    foreach (var value in data) { crc ^= value; for (var i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) == 1 ? 0xedb88320 : 0); }
    return ~crc;
}
