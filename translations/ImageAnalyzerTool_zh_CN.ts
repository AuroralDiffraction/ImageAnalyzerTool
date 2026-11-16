<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN" sourcelanguage="en">
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/ui/ImagePanel.cpp" line="+65"/>
        <source>Image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light frames</source>
        <translation>亮场帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark frames</source>
        <translation>暗场帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Calibrated frames</source>
        <translation>校准帧</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Drop an image here:
a .mat file with an X × Y array or an X × Y × Z array (Z frames),
or one or more TIFF, PNG or JPG images (one frame each)

or click Load…</source>
        <translation>将图像拖放到此处：
包含 X × Y 数组或 X × Y × Z 数组（Z 帧）的 .mat 文件，
或一张或多张 TIFF、PNG、JPG 图像（每张为一帧）

也可以点击“加载…”</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Drop light frames here:
a .mat file with an X × Y × Z array (Z frames),
or one or more TIFF, PNG or JPG images (one frame each)

or click Load…</source>
        <translation>将亮场帧拖放到此处：
包含 X × Y × Z 数组（Z 帧）的 .mat 文件，
或一张或多张 TIFF、PNG、JPG 图像（每张为一帧）

也可以点击“加载…”</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Drop dark frames here:
a .mat file with an X × Y × Z array (Z frames),
or one or more TIFF, PNG or JPG images (one frame each)

In Mean mode, the pixel-by-pixel mean over all frames
becomes the dark frame.

or click Load…</source>
        <translation>将暗场帧拖放到此处：
包含 X × Y × Z 数组（Z 帧）的 .mat 文件，
或一张或多张 TIFF、PNG、JPG 图像（每张为一帧）

在“均值”模式下，所有帧的逐像素均值
用作暗场帧。

也可以点击“加载…”</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Load light frames and dark frames.

This panel then shows
light image − dark image
(the two images on display).

To show a calibrated image from a file instead,
drop a .mat, TIFF, PNG or JPG file here.</source>
        <translation>请加载亮场帧和暗场帧。

此面板随后显示
亮场图像 − 暗场图像
（两个面板当前显示的图像）。

如果要显示文件中的校准图像，
请将 .mat、TIFF、PNG 或 JPG 文件拖放到此处。</translation>
    </message>
</context>
<context>
    <name>iat::AboutDialog</name>
    <message>
        <location filename="../src/ui/AboutDialog.cpp" line="+28"/>
        <source>About %1</source>
        <translation>关于 %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Version %1</source>
        <translation>版本 %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>&lt;p&gt;Dark-frame calibration and noise analysis for image stacks stored in MATLAB .mat files or in TIFF, PNG and JPEG images.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Light frames&lt;/b&gt;: one frame of the X × Y × Z stack, or the mean over Z&lt;br&gt;&lt;b&gt;Dark frames&lt;/b&gt;: the pixel-by-pixel mean over Z, or one frame&lt;br&gt;&lt;b&gt;Calibrated frames&lt;/b&gt;: light image − dark image&lt;/p&gt;&lt;p&gt;Made by %1&lt;br&gt;&lt;a href=&quot;%2&quot;&gt;%3&lt;/a&gt;&lt;/p&gt;&lt;p style=&quot;color:gray&quot;&gt;Built with Qt %4&lt;/p&gt;</source>
        <translation>&lt;p&gt;对存储在 MATLAB .mat 文件或 TIFF、PNG、JPEG 图像中的图像堆栈进行暗场校准和噪声分析。&lt;/p&gt;&lt;p&gt;&lt;b&gt;亮场帧&lt;/b&gt;：X × Y × Z 堆栈中的一帧，或沿 Z 的均值&lt;br&gt;&lt;b&gt;暗场帧&lt;/b&gt;：沿 Z 的逐像素均值，或其中一帧&lt;br&gt;&lt;b&gt;校准帧&lt;/b&gt;：亮场图像 − 暗场图像&lt;/p&gt;&lt;p&gt;作者：%1&lt;br&gt;&lt;a href=&quot;%2&quot;&gt;%3&lt;/a&gt;&lt;/p&gt;&lt;p style=&quot;color:gray&quot;&gt;基于 Qt %4 构建&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Open GitHub page</source>
        <translation>打开 GitHub 页面</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>About Qt</source>
        <translation>关于 Qt</translation>
    </message>
</context>
<context>
    <name>iat::ContrastControl</name>
    <message>
        <location filename="../src/ui/ContrastControl.cpp" line="+49"/>
        <source>Min</source>
        <translation>最小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max</source>
        <translation>最大</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Lowest displayed value: anything at or below it is black</source>
        <translation>显示范围下限：小于或等于该值的像素显示为黑色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Highest displayed value: anything at or above it is white</source>
        <translation>显示范围上限：大于或等于该值的像素显示为白色</translation>
    </message>
</context>
<context>
    <name>iat::HistogramWidget</name>
    <message>
        <location filename="../src/ui/HistogramWidget.cpp" line="+132"/>
        <source>No image</source>
        <translation>无图像</translation>
    </message>
    <message>
        <location line="+74"/>
        <source>pixel count (log)</source>
        <translation>像素数（对数）</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>pixel count</source>
        <translation>像素数</translation>
    </message>
</context>
<context>
    <name>iat::ImageFileReader</name>
    <message>
        <location filename="../src/io/ImageFileReader.cpp" line="+30"/>
        <source>colour image converted to grey with rgb2gray weights (0.2989 R + 0.5870 G + 0.1140 B)</source>
        <translation>彩色图像已按 rgb2gray 权重转换为灰度（0.2989 R + 0.5870 G + 0.1140 B）</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>extra channels (alpha) ignored</source>
        <translation>已忽略额外通道（alpha）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>stored as WhiteIsZero; values used as stored</source>
        <translation>以 WhiteIsZero 方式存储；数值按存储值使用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>page 1 of %1 (only the first page is read)</source>
        <translation>共 %1 页，仅读取第 1 页</translation>
    </message>
    <message>
        <location line="+63"/>
        <source>alpha channel ignored</source>
        <translation>已忽略 alpha 通道</translation>
    </message>
    <message>
        <location line="+61"/>
        <source>No image files given.</source>
        <translation>未指定图像文件。</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>%1 is %2 × %3 pixels, but %4 is %5 × %6. All images must have the same size.</source>
        <translation>%1 为 %2 × %3 像素，而 %4 为 %5 × %6。所有图像的尺寸必须相同。</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>files of different types, converted to double</source>
        <translation>文件类型不同，已转换为 double</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>%1 (%2 of %3 files)</source>
        <translation>%1（%3 个文件中的 %2 个）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Not enough memory to load %1 images.</source>
        <translation>内存不足，无法加载 %1 张图像。</translation>
    </message>
    <message>
        <location line="+81"/>
        <source>%1 is not a supported file. Use a .mat file, or TIFF, PNG or JPG images.</source>
        <translation>不支持文件 %1。请使用 .mat 文件，或 TIFF、PNG、JPG 图像。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>No supported file. Use a .mat file, or TIFF, PNG or JPG images.</source>
        <translation>没有支持的文件。请使用 .mat 文件，或 TIFF、PNG、JPG 图像。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Several files can only be loaded together when they are all TIFF, PNG or JPG images. Load one .mat file at a time.</source>
        <translation>只有全部为 TIFF、PNG 或 JPG 图像时，才能同时加载多个文件。.mat 文件请一次加载一个。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Load one file here. Several images can be combined in the Light frames and Dark frames panels.</source>
        <translation>此处请加载一个文件。多张图像可以在亮场帧和暗场帧面板中合并。</translation>
    </message>
</context>
<context>
    <name>iat::ImagePanel</name>
    <message>
        <location filename="../src/ui/ImagePanel.cpp" line="+80"/>
        <source>Auto calibration off</source>
        <translation>自动校准已关闭</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>This panel shows an image loaded from a file, so it is not calculated from
the light and dark frames.

Click to remove the loaded image and turn automatic calibration
(light image − dark image) back on.</source>
        <translation>此面板显示的是从文件加载的图像，因此不是由亮场帧和暗场帧
计算得到的。

点击可移除已加载的图像，并重新开启自动校准
（亮场图像 − 暗场图像）。</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Single frame</source>
        <translation>单帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Mean</source>
        <translation>均值</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Load…</source>
        <translation>加载…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Open a calibrated image: a .mat, TIFF, PNG or JPG file (or drop one onto this panel).
Automatic calibration then turns off.</source>
        <translation>打开校准图像：.mat、TIFF、PNG 或 JPG 文件（也可以拖放到此面板）。
自动校准随即关闭。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Open a .mat file, or one or more TIFF, PNG or JPG images
(or drop them onto this panel)</source>
        <translation>打开一个 .mat 文件，或一张或多张 TIFF、PNG、JPG 图像
（也可以拖放到此面板）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Light image − dark image</source>
        <translation>亮场图像 − 暗场图像</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>No file loaded</source>
        <translation>未加载文件</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Frame</source>
        <translation>帧</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Previous frame</source>
        <translation>上一帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Next frame</source>
        <translation>下一帧</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Frame number (1-based, as in MATLAB)</source>
        <translation>帧编号（从 1 开始，与 MATLAB 一致）</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Histogram</source>
        <translation>直方图</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Log</source>
        <translation>对数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Logarithmic pixel-count axis</source>
        <translation>像素数坐标轴使用对数刻度</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Stretch the contrast to the 0.5–99.5 % percentiles</source>
        <translation>将对比度拉伸到 0.5–99.5 % 百分位范围</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Full range</source>
        <translation>全范围</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the full data range, from min to max</source>
        <translation>显示完整的数据范围，从最小值到最大值</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Row and column are 1-based, as in MATLAB</source>
        <translation>行和列从 1 开始编号，与 MATLAB 一致</translation>
    </message>
    <message>
        <location line="+26"/>
        <location line="+526"/>
        <source>Mean [DN]</source>
        <translation>均值 [DN]</translation>
    </message>
    <message>
        <location line="-525"/>
        <location line="+525"/>
        <source>Std [DN]</source>
        <translation>标准差 [DN]</translation>
    </message>
    <message>
        <location line="-524"/>
        <location line="+524"/>
        <source>Noise [DN]</source>
        <translation>噪声 [DN]</translation>
    </message>
    <message>
        <location line="-507"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Data (.mat)</source>
        <translation>数据 (.mat)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save raw values to a .mat file: the image on display (double) with Mean, Std and
Noise, or all frames of the stack behind this panel</source>
        <translation>将原始数值保存到 .mat 文件：当前显示的图像（double）及其均值、标准差和
噪声，或此面板对应堆栈的所有帧</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Image on display…</source>
        <translation>当前显示的图像…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>All frames…</source>
        <translation>所有帧…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Image (.png)</source>
        <translation>图像 (.png)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save the image as displayed (current contrast, full resolution) to a PNG file</source>
        <translation>将当前显示的图像（当前对比度、完整分辨率）保存为 PNG 文件</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Show one frame of the stack: %1(:, :, k).
Choose the frame below the image.</source>
        <translation>显示堆栈中的一帧：%1(:, :, k)。
在图像下方选择帧。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the pixel-by-pixel mean over all frames: mean(%1, 3)</source>
        <translation>显示所有帧的逐像素均值：mean(%1, 3)</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>The image has no finite values (all NaN or Inf).</source>
        <translation>图像中没有有限值（全部为 NaN 或 Inf）。</translation>
    </message>
    <message>
        <location line="+86"/>
        <source>of %1</source>
        <translation>/ %1</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Mean mode shows the mean over all frames.
Switch to Single frame to choose one frame.</source>
        <translation>“均值”模式显示所有帧的均值。
切换到“单帧”可选择某一帧。</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>the calibrated image loaded from the file</source>
        <translation>从文件加载的校准图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>light image − dark image (the images on display)</source>
        <translation>亮场图像 − 暗场图像（当前显示的图像）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mean of the image on display, %1

calculateImageStats: mean(double(img), &quot;all&quot;)</source>
        <translation>当前显示图像 %1 的均值

calculateImageStats: mean(double(img), &quot;all&quot;)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Standard deviation over all pixels of the image on display, %1,
normalised by N

calculateImageStats: std(double(img), 1, &quot;all&quot;)</source>
        <translation>当前显示图像 %1 所有像素的标准差，
按 N 归一化

calculateImageStats: std(double(img), 1, &quot;all&quot;)</translation>
    </message>
    <message>
        <location line="+81"/>
        <source>n/a</source>
        <translation>不适用</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Min %1 · Max %2 · %3×%4 px</source>
        <translation>最小 %1 · 最大 %2 · %3×%4 像素</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · %1 NaN/Inf pixels (Mean and Std are then NaN, as in MATLAB)</source>
        <translation> · %1 个 NaN/Inf 像素（此时均值和标准差为 NaN，与 MATLAB 一致）</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Calculating the temporal noise …</source>
        <translation>正在计算时域噪声…</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Temporal noise of the frames behind this panel (calculateImageNoise)</source>
        <translation>此面板对应的所有帧的时域噪声（calculateImageNoise）</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Temporal noise needs at least 2 frames.
(calculateImageNoise returns 0 for a single frame.)</source>
        <translation>时域噪声至少需要 2 帧。
（只有一帧时 calculateImageNoise 返回 0。）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Temporal noise of all %1 frames

calculateImageNoise(frames):
sqrt(mean(var(double(frames), 1, 3), &quot;all&quot;))

It describes the whole stack, so it does not change with the
frame on display or with Single frame / Mean.</source>
        <translation>全部 %1 帧的时域噪声

calculateImageNoise(frames)：
sqrt(mean(var(double(frames), 1, 3), &quot;all&quot;))

它描述的是整个堆栈，因此不随当前显示的帧
或“单帧 / 均值”的切换而变化。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Temporal noise of all %1 light frames

calculateImageNoise(light):
sqrt(mean(var(double(light), 1, 3), &quot;all&quot;))

It describes the whole stack, so it does not change with the
frame on display or with Single frame / Mean.</source>
        <translation>全部 %1 个亮场帧的时域噪声

calculateImageNoise(light)：
sqrt(mean(var(double(light), 1, 3), &quot;all&quot;))

它描述的是整个堆栈，因此不随当前显示的帧
或“单帧 / 均值”的切换而变化。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Temporal noise of all %1 dark frames

calculateImageNoise(dark):
sqrt(mean(var(double(dark), 1, 3), &quot;all&quot;))

It describes the whole stack, so it does not change with the
frame on display or with Single frame / Mean.</source>
        <translation>全部 %1 个暗场帧的时域噪声

calculateImageNoise(dark)：
sqrt(mean(var(double(dark), 1, 3), &quot;all&quot;))

它描述的是整个堆栈，因此不随当前显示的帧
或“单帧 / 均值”的切换而变化。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Temporal noise of the %1 frames loaded from the file

calculateImageNoise(calibrated):
sqrt(mean(var(double(calibrated), 1, 3), &quot;all&quot;))</source>
        <translation>从文件加载的 %1 帧的时域噪声

calculateImageNoise(calibrated)：
sqrt(mean(var(double(calibrated), 1, 3), &quot;all&quot;))</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Temporal noise of the %1 calibrated frames light(:, :, k) − D, where D is the
dark image on display (mean(dark, 3) or one dark frame)

calculateImageNoise(light − D):
sqrt(mean(var(light − D, 1, 3), &quot;all&quot;))

Subtracting the same image D from every light frame does not change
the temporal variance, so this equals the light-frame noise.</source>
        <translation>%1 个校准帧 light(:, :, k) − D 的时域噪声，其中 D 为
当前显示的暗场图像（mean(dark, 3) 或某一暗场帧）

calculateImageNoise(light − D)：
sqrt(mean(var(light − D, 1, 3), &quot;all&quot;))

从每个亮场帧中减去同一幅图像 D 不会改变
时域方差，因此该值等于亮场帧的噪声。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Hover to read pixel values · wheel to zoom · double-click to fit · right-click to copy</source>
        <translation>悬停读取像素值 · 滚轮缩放 · 双击适应窗口 · 右键复制</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Row %1, Col %2:  %3</source>
        <translation>行 %1，列 %2：  %3</translation>
    </message>
    <message>
        <location line="+100"/>
        <source>Display range %1 to %2 · Min %3 · Max %4 · %5×%6 px</source>
        <translation>显示范围 %1 至 %2 · 最小 %3 · 最大 %4 · %5×%6 像素</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · noise over %1 frames</source>
        <translation> · %1 帧的噪声</translation>
    </message>
    <message>
        <location line="+13"/>
        <location line="+14"/>
        <source>image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="-14"/>
        <source>%1 image</source>
        <translation>%1图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copied the %1 to the clipboard (%2 × %3 px)</source>
        <translation>已将%1复制到剪贴板（%2 × %3 像素）</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Copied the %1 figure (image, histogram and statistics) to the clipboard (%2 × %3 px)</source>
        <translation>已将%1的汇总图（图像、直方图和统计值）复制到剪贴板（%2 × %3 像素）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Copy image</source>
        <translation>复制图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy image with histogram and statistics</source>
        <translation>复制图像及直方图和统计值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Save image as PNG…</source>
        <translation>将图像另存为 PNG…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save image on display as .mat…</source>
        <translation>将当前显示的图像保存为 .mat…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save all frames as .mat…</source>
        <translation>将所有帧保存为 .mat…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fit to window</source>
        <translation>适应窗口</translation>
    </message>
</context>
<context>
    <name>iat::MainWindow</name>
    <message>
        <location filename="../src/ui/MainWindow.cpp" line="+25"/>
        <source>Image Analyzer Tool</source>
        <translation>Image Analyzer Tool</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>New tab (Ctrl+T)</source>
        <translation>新建标签页 (Ctrl+T)</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Drop a .mat file, or one or more TIFF, PNG or JPG images, onto the panel. For light, dark and calibrated frames, use the switch at the top right.</source>
        <translation>将一个 .mat 文件，或一张或多张 TIFF、PNG、JPG 图像拖放到面板上。如需亮场帧、暗场帧和校准帧，请使用右上角的切换开关。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;File</source>
        <translation>文件(&amp;F)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;New tab</source>
        <translation>新建标签页(&amp;N)</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+72"/>
        <source>Open &amp;light frames…</source>
        <translation>打开亮场帧(&amp;L)…</translation>
    </message>
    <message>
        <location line="-69"/>
        <source>Ctrl+L</source>
        <translation>Ctrl+L</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open &amp;dark frames…</source>
        <translation>打开暗场帧(&amp;D)…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ctrl+D</source>
        <translation>Ctrl+D</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open &amp;calibrated image…</source>
        <translation>打开校准图像(&amp;A)…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>&amp;Close tab</source>
        <translation>关闭标签页(&amp;C)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Rename tab…</source>
        <translation>重命名标签页(&amp;R)…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>&amp;Settings…</source>
        <translation>设置(&amp;S)…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>E&amp;xit</source>
        <translation>退出(&amp;X)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&amp;Help</source>
        <translation>帮助(&amp;H)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;GitHub: %1</source>
        <translation>&amp;GitHub：%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;About Image Analyzer Tool…</source>
        <translation>关于 Image Analyzer Tool(&amp;A)…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>About &amp;Qt</source>
        <translation>关于 Qt(&amp;Q)</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Could not restart. Close the app and open it again to change the language.</source>
        <translation>无法重新启动。请关闭程序后重新打开，以切换语言。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Session %1</source>
        <translation>会话 %1</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Open &amp;image…</source>
        <translation>打开图像(&amp;I)…</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Rename tab</source>
        <translation>重命名标签页</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Tab name:</source>
        <translation>标签页名称：</translation>
    </message>
</context>
<context>
    <name>iat::MatFileReader</name>
    <message>
        <location filename="../src/io/MatFileReader.cpp" line="+52"/>
        <source>the file is truncated or corrupt</source>
        <translation>文件被截断或已损坏</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>corrupt small data element</source>
        <translation>小数据元素已损坏</translation>
    </message>
    <message>
        <location line="+84"/>
        <source>a compressed variable could not be decompressed (corrupt file or not enough memory)</source>
        <translation>无法解压某个压缩变量（文件损坏或内存不足）</translation>
    </message>
    <message>
        <location line="+9"/>
        <location line="+56"/>
        <source>empty array</source>
        <translation>空数组</translation>
    </message>
    <message>
        <location line="-49"/>
        <source>unexpected array flags</source>
        <translation>数组标志异常</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>MATLAB objects are not supported</source>
        <translation>不支持 MATLAB 对象</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>unexpected dimensions element</source>
        <translation>维度元素异常</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>negative array dimension</source>
        <translation>数组维度为负数</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>unexpected array name element</source>
        <translation>数组名称元素异常</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 arrays are not supported</source>
        <translation>不支持 %1 数组</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>complex arrays are not supported</source>
        <translation>不支持复数数组</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <source>array is too large</source>
        <translation>数组过大</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>arrays with more than 3 dimensions are not supported</source>
        <translation>不支持超过 3 维的数组</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>unsupported storage type for numeric data</source>
        <translation>数值数据的存储类型不受支持</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>numeric data size does not match the array dimensions</source>
        <translation>数值数据的大小与数组维度不符</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>could not create the image stack</source>
        <translation>无法创建图像堆栈</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The file is too small to be a MAT-file.</source>
        <translation>文件太小，不是 MAT 文件。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>This is a MATLAB v7.3 (HDF5) MAT-file, which this version cannot read yet. Re-save it in MATLAB with:  save(&apos;file.mat&apos;, &apos;variableName&apos;, &apos;-v7&apos;)</source>
        <translation>这是 MATLAB v7.3（HDF5）格式的 MAT 文件，当前版本暂不支持读取。请在 MATLAB 中用以下命令重新保存：  save(&apos;file.mat&apos;, &apos;variableName&apos;, &apos;-v7&apos;)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is not a MATLAB MAT-file (version 5 or 7 format).</source>
        <translation>这不是 MATLAB MAT 文件（版本 5 或 7 格式）。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>The MAT-file header has an invalid byte-order marker.</source>
        <translation>MAT 文件头中的字节序标记无效。</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Could not read the MAT-file: %1.</source>
        <translation>无法读取 MAT 文件：%1。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Not enough memory to read this MAT-file.</source>
        <translation>内存不足，无法读取此 MAT 文件。</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>Could not open the file: %1</source>
        <translation>无法打开文件：%1</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Not enough memory to read this file.</source>
        <translation>内存不足，无法读取此文件。</translation>
    </message>
</context>
<context>
    <name>iat::MatFileWriter</name>
    <message>
        <location filename="../src/io/MatFileWriter.cpp" line="+148"/>
        <source>&apos;%1&apos; is not a valid MATLAB variable name.</source>
        <translation>“%1”不是有效的 MATLAB 变量名。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Variable &apos;%1&apos; has an invalid size.</source>
        <translation>变量“%1”的大小无效。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Variable &apos;%1&apos; is larger than 2 GB, the limit of MATLAB -v7 files. Save fewer frames at a time.</source>
        <translation>变量“%1”超过 2 GB，这是 MATLAB -v7 文件的上限。请每次保存较少的帧。</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+84"/>
        <source>Not enough memory to save &apos;%1&apos;.</source>
        <translation>内存不足，无法保存“%1”。</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>Could not compress &apos;%1&apos; (not enough memory?).</source>
        <translation>无法压缩“%1”（内存不足？）。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Variable &apos;%1&apos; is too large.</source>
        <translation>变量“%1”过大。</translation>
    </message>
    <message>
        <location line="+28"/>
        <location line="+15"/>
        <source>Variable &apos;%1&apos; is empty.</source>
        <translation>变量“%1”为空。</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Could not create %1: %2</source>
        <translation>无法创建 %1：%2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Could not write %1: %2</source>
        <translation>无法写入 %1：%2</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Could not save %1: %2</source>
        <translation>无法保存 %1：%2</translation>
    </message>
</context>
<context>
    <name>iat::ModeSwitch</name>
    <message>
        <location filename="../src/ui/ModeSwitch.cpp" line="+55"/>
        <source>Panel layout</source>
        <translation>面板布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Single image, or three panels: light, dark and calibrated frames</source>
        <translation>单图像，或三个面板：亮场帧、暗场帧和校准帧</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Single image: one panel</source>
        <translation>单图像：一个面板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Three panels: light frames, dark frames and calibrated frames</source>
        <translation>三个面板：亮场帧、暗场帧和校准帧</translation>
    </message>
</context>
<context>
    <name>iat::SessionTab</name>
    <message>
        <location filename="../src/ui/SessionTab.cpp" line="+161"/>
        <location line="+248"/>
        <source>image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="-248"/>
        <location line="+385"/>
        <source>light frames</source>
        <translation>亮场帧</translation>
    </message>
    <message>
        <location line="-384"/>
        <location line="+387"/>
        <source>dark frames</source>
        <translation>暗场帧</translation>
    </message>
    <message>
        <location line="-384"/>
        <source>calibrated frames</source>
        <translation>校准帧</translation>
    </message>
    <message>
        <location line="+51"/>
        <source>Single image</source>
        <translation>单图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Three panels: light frames, dark frames and calibrated frames (light − dark)</source>
        <translation>三个面板：亮场帧、暗场帧和校准帧（亮场 − 暗场）</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>MATLAB data and images (*.mat %1);;MATLAB data (*.mat);;Images (%1);;All files (*)</source>
        <translation>MATLAB 数据和图像 (*.mat %1);;MATLAB 数据 (*.mat);;图像 (%1);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Open an image: one .mat file, or one or more images</source>
        <translation>打开图像：一个 .mat 文件，或一张或多张图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open light frames: one .mat file, or one or more images</source>
        <translation>打开亮场帧：一个 .mat 文件，或一张或多张图像</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Open dark frames: one .mat file, or one or more images</source>
        <translation>打开暗场帧：一个 .mat 文件，或一张或多张图像</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Open a calibrated image (turns automatic calibration off)</source>
        <translation>打开校准图像（将关闭自动校准）</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Cannot load these files</source>
        <translation>无法加载这些文件</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>%1 images</source>
        <translation>%1 张图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Reading %1 …</source>
        <translation>正在读取 %1…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source> (%1 of another type ignored)</source>
        <translation>（已忽略其他类型的 %1）</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>file</source>
        <translation>个文件</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>files</source>
        <translation>个文件</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>No image data</source>
        <translation>没有图像数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 does not contain a numeric X × Y or X × Y × Z array.

Variables in the file:
%2</source>
        <translation>%1 中没有 X × Y 或 X × Y × Z 的数值数组。

文件中的变量：
%2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>  (none)</source>
        <translation>  （无）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Choose a variable</source>
        <translation>选择变量</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 contains several arrays.
Which one holds the %2?</source>
        <translation>%1 包含多个数组。
哪一个是%2？</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Could not open file</source>
        <translation>无法打开文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Could not open %1</source>
        <translation>无法打开 %1</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Could not open images</source>
        <translation>无法打开图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Could not open the images: %1</source>
        <translation>无法打开图像：%1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>%1 … %2 · %3 %4 files</source>
        <translation>%1 … %2 · %3 个%4文件</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Loaded a calibrated image: %1 · automatic calibration is off</source>
        <translation>已加载校准图像：%1 · 自动校准已关闭</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Loaded %1: %2</source>
        <translation>已加载%1：%2</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Calculated noise of %1 [DN]: %2</source>
        <translation>%1的噪声 [DN]：%2</translation>
    </message>
    <message>
        <location line="+9"/>
        <source> · mean of %1</source>
        <translation> · %1的均值</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+29"/>
        <source>frame</source>
        <translation>帧</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+29"/>
        <source>frames</source>
        <translation>帧</translation>
    </message>
    <message>
        <location line="-14"/>
        <source>No file loaded</source>
        <translation>未加载文件</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Calculating the mean of %1 …</source>
        <translation>正在计算 %1的均值…</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Automatic calibration is on: light image − dark image</source>
        <translation>自动校准已开启：亮场图像 − 暗场图像</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Mean of %1</source>
        <translation>%1的均值</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>light frame</source>
        <translation>亮场帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Light frame %1 of %2</source>
        <translation>亮场帧 %1/%2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>mean of %1</source>
        <translation>%1的均值</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>dark frame</source>
        <translation>暗场帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>dark frame %1 of %2</source>
        <translation>暗场帧 %1/%2</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Dark frames ready.

Now load light frames to compute
light image − dark image.</source>
        <translation>暗场帧已就绪。

请加载亮场帧以计算
亮场图像 − 暗场图像。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light frames ready.

Now load dark frames to compute
light image − dark image.</source>
        <translation>亮场帧已就绪。

请加载暗场帧以计算
亮场图像 − 暗场图像。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Waiting for the mean light and dark images …</source>
        <translation>正在等待亮场和暗场的均值图像…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Waiting for the mean dark image …</source>
        <translation>正在等待暗场均值图像…</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Waiting for the mean light image …</source>
        <translation>正在等待亮场均值图像…</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+10"/>
        <source>Light image − dark image</source>
        <translation>亮场图像 − 暗场图像</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>The sizes do not match:

light image  %1 × %2
dark image   %3 × %4

Load files with the same X × Y size.</source>
        <translation>尺寸不一致：

亮场图像  %1 × %2
暗场图像  %3 × %4

请加载 X × Y 尺寸相同的文件。</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>Save image as PNG</source>
        <translation>将图像另存为 PNG</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Save %1 image as PNG</source>
        <translation>将%1图像另存为 PNG</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>PNG image (*.png)</source>
        <translation>PNG 图像 (*.png)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Save all frames as .mat</source>
        <translation>将所有帧保存为 .mat</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Save all %1 as .mat</source>
        <translation>将全部%1保存为 .mat</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Save image data as .mat</source>
        <translation>将图像数据保存为 .mat</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Save %1 data as .mat</source>
        <translation>将%1数据保存为 .mat</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>MATLAB data (*.mat)</source>
        <translation>MATLAB 数据 (*.mat)</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Saved %1</source>
        <translation>已保存 %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Could not save</source>
        <translation>无法保存</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Could not save %1</source>
        <translation>无法保存 %1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>There is no image to save.</source>
        <translation>没有可保存的图像。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Could not write %1.</source>
        <translation>无法写入 %1。</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Saving %1 …</source>
        <translation>正在保存 %1…</translation>
    </message>
</context>
<context>
    <name>iat::SettingsDialog</name>
    <message>
        <location filename="../src/ui/SettingsDialog.cpp" line="+18"/>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>A new language is used after the app restarts.</source>
        <translation>新的语言在程序重新启动后生效。</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Restart to change the language</source>
        <translation>重新启动以切换语言</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The interface changes to %1 when the app restarts.

Restarting closes all tabs; images that are loaded are not kept.</source>
        <translation>程序重新启动后，界面将切换为%1。

重新启动会关闭所有标签页，已加载的图像不会保留。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Restart now</source>
        <translation>立即重启</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Later</source>
        <translation>稍后</translation>
    </message>
</context>
<context>
    <name>iat::TiffReader</name>
    <message>
        <location filename="../src/io/TiffReader.cpp" line="+75"/>
        <source>type %1</source>
        <translation>类型 %1</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>the file is truncated or corrupt</source>
        <translation>文件被截断或已损坏</translation>
    </message>
    <message>
        <location line="+27"/>
        <location line="+41"/>
        <source>the file is corrupt</source>
        <translation>文件已损坏</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>the file is corrupt (unexpected field type)</source>
        <translation>文件已损坏（字段类型异常）</translation>
    </message>
    <message>
        <location line="+61"/>
        <location line="+5"/>
        <source>corrupt PackBits data</source>
        <translation>PackBits 数据已损坏</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>old-style LZW compression (from before TIFF 6.0) is not supported</source>
        <translation>不支持旧式 LZW 压缩（TIFF 6.0 之前）</translation>
    </message>
    <message>
        <location line="+50"/>
        <location line="+22"/>
        <source>corrupt LZW data</source>
        <translation>LZW 数据已损坏</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Deflate-compressed image data could not be decompressed (corrupt file?)</source>
        <translation>无法解压 Deflate 压缩的图像数据（文件损坏？）</translation>
    </message>
    <message>
        <location line="+46"/>
        <location line="+7"/>
        <location line="+14"/>
        <source>not a TIFF file</source>
        <translation>不是 TIFF 文件</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>unsupported BigTIFF variant</source>
        <translation>不支持的 BigTIFF 变体</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>the image has no width or height</source>
        <translation>图像没有宽度或高度</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+79"/>
        <source>the image is too large</source>
        <translation>图像过大</translation>
    </message>
    <message>
        <location line="-73"/>
        <source>unsupported number of samples per pixel</source>
        <translation>每像素采样数不受支持</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>the file is corrupt (no bit depth)</source>
        <translation>文件已损坏（缺少位深度）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>channels with different bit depths are not supported</source>
        <translation>不支持各通道位深度不同的图像</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>channels with different sample formats are not supported</source>
        <translation>不支持各通道采样格式不同的图像</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>palette (indexed colour) TIFF images are not supported; save the image as greyscale</source>
        <translation>不支持调色板（索引颜色）TIFF 图像；请将图像保存为灰度图</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>transparency-mask TIFF images are not supported</source>
        <translation>不支持透明蒙版 TIFF 图像</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>%1 TIFF images are not supported; save the image as greyscale or RGB</source>
        <translation>不支持 %1 TIFF 图像；请将图像保存为灰度或 RGB</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>this TIFF colour space is not supported; save the image as greyscale or RGB</source>
        <translation>不支持此 TIFF 颜色空间；请将图像保存为灰度或 RGB</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>the file is corrupt (RGB image with fewer than 3 samples per pixel)</source>
        <translation>文件已损坏（RGB 图像每像素采样数少于 3）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1-bit samples are not supported (only 1, 8, 16, 32 and 64 bits)</source>
        <translation>不支持 %1 位采样（仅支持 1、8、16、32 和 64 位）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>1-bit images with more than one channel are not supported</source>
        <translation>不支持多通道的 1 位图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>complex-valued TIFF images are not supported</source>
        <translation>不支持复数 TIFF 图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1-bit floating-point samples are not supported (only 32 and 64 bits)</source>
        <translation>不支持 %1 位浮点采样（仅支持 32 和 64 位）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 compression is not supported. Save the image uncompressed, or with LZW or Deflate (ZIP) compression.</source>
        <translation>不支持 %1 压缩。请将图像保存为未压缩格式，或使用 LZW 或 Deflate (ZIP) 压缩。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>unsupported predictor</source>
        <translation>不支持的预测器</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>the file is corrupt (predictor does not match the sample format)</source>
        <translation>文件已损坏（预测器与采样格式不匹配）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>bit-reversed (FillOrder 2) TIFF files are not supported</source>
        <translation>不支持位序反转（FillOrder 2）的 TIFF 文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>unsupported planar configuration</source>
        <translation>不支持的平面配置</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>the file is corrupt (bad tile size)</source>
        <translation>文件已损坏（瓦片尺寸错误）</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>the file is corrupt (missing strip or tile offsets)</source>
        <translation>文件已损坏（缺少条带或瓦片偏移）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>the file is corrupt (missing strip or tile sizes)</source>
        <translation>文件已损坏（缺少条带或瓦片大小）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>the file is corrupt (no strip or tile sizes)</source>
        <translation>文件已损坏（没有条带或瓦片大小）</translation>
    </message>
    <message>
        <location line="+22"/>
        <location line="+20"/>
        <source>the image data is truncated</source>
        <translation>图像数据被截断</translation>
    </message>
    <message>
        <location line="+61"/>
        <source>Not enough memory to read the image</source>
        <translation>内存不足，无法读取图像</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../src/main.cpp" line="+30"/>
        <source>Dark-frame calibration and noise analysis for image stacks (MATLAB .mat, TIFF, PNG, JPEG)</source>
        <translation>对图像堆栈进行暗场校准和噪声分析（MATLAB .mat、TIFF、PNG、JPEG）</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Image to open in the first tab (the light frames, when dark frames are given too): a .mat file, or an image file (repeat the option for several images).</source>
        <translation>在第一个标签页中打开的图像（同时指定暗场帧时作为亮场帧）：一个 .mat 文件，或一个图像文件（多张图像可重复使用该选项）。</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+5"/>
        <location line="+5"/>
        <source>file</source>
        <translation>文件</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Dark frames to open in the first tab (switches it to three panels): a .mat file, or an image file (repeat the option for several images).</source>
        <translation>在第一个标签页中打开的暗场帧（会切换到三面板）：一个 .mat 文件，或一个图像文件（多张图像可重复使用该选项）。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Calibrated image to show in the first tab (switches it to three panels and turns automatic calibration off).</source>
        <translation>在第一个标签页中显示的校准图像（会切换到三面板并关闭自动校准）。</translation>
    </message>
</context>
</TS>
