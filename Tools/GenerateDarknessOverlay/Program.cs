using System;
using System.IO;
using System.IO.Compression;
using System.Text;

const int Size = 1024;
var pixels = new byte[Size * Size * 4];
for (var y = 0; y < Size; ++y)
for (var x = 0; x < Size; ++x)
{
    var nx = (x + 0.5f) / Size - 0.5f;
    var ny = (y + 0.5f) / Size - 0.5f;
    var radius = MathF.Sqrt(nx * nx + ny * ny);
    // A tight clear area around the player, followed by a short dark falloff.
    var alpha = Math.Clamp((radius - 0.08f) / 0.24f, 0f, 1f);
    alpha = alpha * alpha * (3f - 2f * alpha);
    var offset = (y * Size + x) * 4;
    pixels[offset + 3] = (byte)(alpha * 235f);
}

using var output = File.Create(args[0]);
output.Write(new byte[] { 137, 80, 78, 71, 13, 10, 26, 10 });
WriteChunk(output, "IHDR", Header());
using var raw = new MemoryStream();
for (var y = 0; y < Size; ++y) { raw.WriteByte(0); raw.Write(pixels, y * Size * 4, Size * 4); }
using var compressed = new MemoryStream();
using (var zlib = new ZLibStream(compressed, CompressionLevel.SmallestSize, true)) raw.WriteTo(zlib);
WriteChunk(output, "IDAT", compressed.ToArray());
WriteChunk(output, "IEND", Array.Empty<byte>());

byte[] Header()
{
    var data = new byte[13];
    WriteBuffer(data, 0, Size); WriteBuffer(data, 4, Size); data[8] = 8; data[9] = 6;
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
