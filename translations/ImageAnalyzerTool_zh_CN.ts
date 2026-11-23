<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN" sourcelanguage="en">
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/ui/ImagePanel.cpp" line="+74"/>
        <source>Image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light frames</source>
        <translation>场景帧</translation>
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
        <translation>将场景帧拖放到此处：
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
        <translation>请加载场景帧和暗场帧。

此面板随后显示
场景图像 − 暗场图像
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
        <translation>&lt;p&gt;对存储在 MATLAB .mat 文件或 TIFF、PNG、JPEG 图像中的图像堆栈进行暗场校准和噪声分析。&lt;/p&gt;&lt;p&gt;&lt;b&gt;场景帧&lt;/b&gt;：X × Y × Z 堆栈中的一帧，或沿 Z 的均值&lt;br&gt;&lt;b&gt;暗场帧&lt;/b&gt;：沿 Z 的逐像素均值，或其中一帧&lt;br&gt;&lt;b&gt;校准帧&lt;/b&gt;：场景图像 − 暗场图像&lt;/p&gt;&lt;p&gt;作者：%1&lt;br&gt;&lt;a href=&quot;%2&quot;&gt;%3&lt;/a&gt;&lt;/p&gt;&lt;p style=&quot;color:gray&quot;&gt;基于 Qt %4 构建&lt;/p&gt;</translation>
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
    <name>iat::CorrectionDialog</name>
    <message>
        <location filename="../src/ui/CorrectionDialog.cpp" line="+33"/>
        <source>Correction settings</source>
        <translation>校正设置</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Spectral response</source>
        <translation>光谱响应</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No spectral response loaded.</source>
        <translation>未加载光谱响应。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Spectral correction</source>
        <translation>光谱校正</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Correct the channels</source>
        <translation>校正通道</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fabry-Perot channels also respond outside their peak (a broad background shared by all channels, harmonics, cross-talk). The correction turns the 16 measured channels into bands that respond like narrow filters at the channel peaks.</source>
        <translation>法布里-珀罗通道在峰值之外也有响应（所有通道共有的宽背景、谐波、串扰）。校正将 16 个实测通道转换为在通道峰值处如窄带滤光片般响应的波段。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Compute from the spectral response</source>
        <translation>由光谱响应计算</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Correction matrix from a file</source>
        <translation>从文件读取校正矩阵</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>measured widths</source>
        <translation>实测宽度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Width (FWHM) of the target band of each channel; narrower bands separate wavelengths better but amplify noise</source>
        <translation>每个通道目标波段的宽度（半高宽）；波段越窄，波长分辨越好，但噪声放大越多</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Regularisation: larger keeps noise low, smaller follows the targets more closely</source>
        <translation>正则化：越大噪声越低，越小越贴近目标</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Target width:</source>
        <translation>目标宽度：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Regularisation:</source>
        <translation>正则化：</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Load matrix…</source>
        <translation>加载矩阵…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A V × 16 matrix supplied with the camera (.csv or .mat); with 17 columns the first is the band&apos;s centre wavelength</source>
        <translation>相机附带的 V × 16 矩阵（.csv 或 .mat）；若有 17 列，第一列为波段中心波长</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Reference frames</source>
        <translation>参考帧</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>same</source>
        <translation>相同</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Exposure time of the image; with the white frame&apos;s, the two are compared per ms</source>
        <translation>图像的曝光时间；与白参考帧的曝光时间一起，按每毫秒进行比较</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Reflectance of the white reference (e.g. 0.99 for Spectralon)</source>
        <translation>白参考的反射率（例如 Spectralon 为 0.99）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Image exposure:</source>
        <translation>图像曝光：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>White exposure:</source>
        <translation>白参考曝光：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>White reflectance:</source>
        <translation>白参考反射率：</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Measured channel responses</source>
        <translation>实测通道响应</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Corrected band responses</source>
        <translation>校正后的波段响应</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Response</source>
        <translation>响应</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Load a spectral response to see the channel responses.</source>
        <translation>加载光谱响应以查看通道响应。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Save sensor profile…</source>
        <translation>保存传感器配置…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Mosaic layout, bit depth, these settings and the paths of the spectral response, correction matrix and reference frames, in one file</source>
        <translation>马赛克排列、位深、这些设置以及光谱响应、校正矩阵和参考帧的路径，保存在一个文件中</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Load sensor profile…</source>
        <translation>加载传感器配置…</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>No matrix file.</source>
        <translation>无矩阵文件。</translation>
    </message>
    <message>
        <location line="+61"/>
        <source>Load correction matrix</source>
        <translation>加载校正矩阵</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Correction matrix (*.csv *.txt *.mat);;All files (*)</source>
        <translation>校正矩阵 (*.csv *.txt *.mat);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Load a matrix file.</source>
        <translation>请加载矩阵文件。</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Load a spectral response first.</source>
        <translation>请先加载光谱响应。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Matrix from %1: %2 → %3 bands, noise × %4.</source>
        <translation>来自 %1 的矩阵：%2 → %3 个波段，噪声 × %4。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Match to the target bands: %1 % residual; noise × %2. The narrower the bands, the more noise: raise the regularisation (or the width) if the corrected image is noisy, or denoise after the correction (MNF, guided filter).</source>
        <translation>与目标波段的匹配：残差 %1 %；噪声 × %2。波段越窄，噪声越大：若校正后图像噪声大，请增大正则化（或宽度），或在校正后去噪（MNF、引导滤波）。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Channel %1</source>
        <translation>通道 %1</translation>
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
    <name>iat::HsiWorkspace</name>
    <message>
        <location filename="../src/ui/HsiWorkspace.cpp" line="+141"/>
        <source>Classes (share of pixels): %1</source>
        <translation>类别（像素占比）：%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Clustering did not give a result.</source>
        <translation>聚类没有得到结果。</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Drop a raw 4 × 4 mosaic here (TIFF, PNG or .mat), or a cube (ENVI .hdr, .mat),
or use “Import from workspace” to take the image or calibrated image of a tab.

A spectral response file (.mat with lambda and QE_percent, or .csv) can be dropped too.</source>
        <translation>将原始 4 × 4 马赛克（TIFF、PNG 或 .mat）或数据立方体（ENVI .hdr、.mat）拖放到这里，
或使用“从工作区导入”获取某个标签页的图像或校准图像。

也可以拖入光谱响应文件（含 lambda 和 QE_percent 的 .mat，或 .csv）。</translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Processing…</source>
        <translation>处理中…</translation>
    </message>
    <message>
        <location line="+78"/>
        <source>Source</source>
        <translation>数据来源</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Open file…</source>
        <translation>打开文件…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A raw mosaic (TIFF, PNG, JPEG, .mat), a cube (ENVI .hdr, .mat) or a spectral response file</source>
        <translation>原始马赛克（TIFF、PNG、JPEG、.mat）、数据立方体（ENVI .hdr、.mat）或光谱响应文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Import from workspace…</source>
        <translation>从工作区导入…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Use the single image, light or dark frames, or calibrated image of an open tab</source>
        <translation>使用已打开标签页的单图像、场景帧、暗场帧或校准图像</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No image yet. Drop a file onto the workspace.</source>
        <translation>尚无图像。请将文件拖放到工作区。</translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Mosaic → cube</source>
        <translation>马赛克 → 数据立方体</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Mosaic layout…</source>
        <translation>马赛克排列…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Which channel sits at each position of the 4 × 4 super-pixel</source>
        <translation>4 × 4 超像素中每个位置对应的通道</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Sub-sample (as extract_hsi_channels.m)</source>
        <translation>子采样（同 extract_hsi_channels.m）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sub-sample + registration (recommended)</source>
        <translation>子采样 + 配准（推荐）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full resolution: bilinear</source>
        <translation>全分辨率：双线性</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full resolution: PPID demosaic</source>
        <translation>全分辨率：PPID 去马赛克</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>One value per super-pixel and channel: rows/4 × cols/4 × 16. The bands are up to ¾ of a cube pixel apart, which gives false spectra at edges.</source>
        <translation>每个超像素每个通道一个值：行/4 × 列/4 × 16。各波段之间最多相差 ¾ 个立方体像素，会在边缘产生虚假光谱。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>As sub-sampling, but every band is moved to the centre of the super-pixel, so all bands describe the same point.</source>
        <translation>与子采样相同，但每个波段都移到超像素中心，使所有波段描述同一个点。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Every band interpolated to the full sensor resolution from its own pixels.</source>
        <translation>每个波段由其自身像素插值到传感器全分辨率。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pseudo-panchromatic image difference (Mihoubi et al. 2017): a full-resolution image of all channels guides the interpolation, so edges stay sharp and aligned.</source>
        <translation>伪全色图像差分（Mihoubi 等，2017）：由所有通道合成的全分辨率图像引导插值，使边缘保持清晰且对齐。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Method:</source>
        <translation>方法：</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+2661"/>
        <source>Dark offset in DN, subtracted from every pixel. Leave 0 for a calibrated image (light − dark), which has it removed already.</source>
        <translation>暗电平偏移（DN），从每个像素中减去。校准图像（场景 − 暗场）已去除偏移，请保留为 0。</translation>
    </message>
    <message>
        <location line="-2659"/>
        <source>Black level (DN):</source>
        <translation>黑电平 (DN)：</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>no check</source>
        <translation>不检查</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ADC bit depth. Values above 2^bits − 1 are taken as MSB-aligned in a 16-bit container and shifted down, as hsi_reconstruct.m does.</source>
        <translation>ADC 位深。超过 2^位数 − 1 的数值视为存放在 16 位容器中的高位对齐数据并右移，与 hsi_reconstruct.m 相同。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bit depth:</source>
        <translation>位深：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clip negative values to 0</source>
        <translation>将负值截为 0</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Processing</source>
        <translation>处理</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2230"/>
        <source>Reflectance</source>
        <translation>反射率</translation>
    </message>
    <message>
        <location line="-2229"/>
        <source>Divide every band by its mean over a white reference area: spectra become comparable between exposures and lights. Recommended first step.</source>
        <translation>将每个波段除以其在白参考区域内的均值：使不同曝光和光源下的光谱可以比较。推荐作为第一步。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+217"/>
        <source>Pick white area</source>
        <translation>选择白参考区域</translation>
    </message>
    <message>
        <location line="-216"/>
        <source>Then drag a rectangle over a white (or known grey) reference in the image</source>
        <translation>然后在图像中的白色（或已知灰度）参考物上拖出一个矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear</source>
        <translation>清除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove the white area and turn reflectance off</source>
        <translation>删除白参考区域并关闭反射率</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Reflectance of the reference (e.g. 0.99 for Spectralon, about 0.9 for white paper)</source>
        <translation>参考物的反射率（例如 Spectralon 为 0.99，白纸约为 0.9）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reference reflectance:</source>
        <translation>参考反射率：</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+751"/>
        <source>No white area chosen.</source>
        <translation>尚未选择白参考区域。</translation>
    </message>
    <message>
        <location line="-748"/>
        <source>Spectral denoise (MNF)</source>
        <translation>光谱降噪 (MNF)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Minimum noise fraction, as in ENVI: the bands are turned into components ordered by signal-to-noise ratio, the noisiest are dropped, and the rest turned back into bands.</source>
        <translation>最小噪声分离（与 ENVI 相同）：将波段变换为按信噪比排序的分量，去掉噪声最大的分量，再将其余分量变换回波段。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Number of components to keep</source>
        <translation>保留的分量数</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Edge-preserving denoise (guided)</source>
        <translation>保边降噪（引导滤波）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Guided filter with the mean of all bands as the guide: smooths noise while keeping the edges of every band in the same place.</source>
        <translation>以所有波段的均值图像为引导的引导滤波：平滑噪声，同时使各波段的边缘保持在同一位置。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>r </source>
        <translation>半径 </translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Anti-aliasing (low-pass)</source>
        <translation>抗混叠（低通）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Gaussian low-pass of every band: removes the coloured fringes that sampling every band at 1/16 of the pixels causes. Registration or demosaicking helps more.</source>
        <translation>对每个波段做高斯低通：去除每个波段只采样 1/16 像素所造成的彩色条纹。配准或去马赛克效果更好。</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>Enlarge every band. Interpolation only: no new detail. For more resolution, choose a full-resolution extraction method instead.</source>
        <translation>放大每个波段。仅为插值，不会产生新细节。如需更高分辨率，请改用全分辨率提取方法。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Nearest</source>
        <translation>最近邻</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Bilinear</source>
        <translation>双线性</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Bicubic</source>
        <translation>双三次</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Upscale:</source>
        <translation>放大：</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Colour reconstruction</source>
        <translation>颜色重建</translation>
    </message>
    <message>
        <location line="-262"/>
        <source>Load spectral response…</source>
        <translation>加载光谱响应…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The reconstruction matrix: QE of every channel (.mat with lambda and QE_percent, or .csv with Band1 … Band16 columns). It stays on your computer; it is not copied or exported.</source>
        <translation>重建矩阵：每个通道的量子效率（含 lambda 和 QE_percent 的 .mat，或含 Band1 … Band16 列的 .csv）。该文件只留在您的电脑上，不会被复制或导出。</translation>
    </message>
    <message>
        <location line="+267"/>
        <source>Correlation length of the smoothness prior of the spectral estimate</source>
        <translation>光谱估计的平滑先验的相关长度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Smoothness σ:</source>
        <translation>平滑度 σ：</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Regularisation, relative to the mean channel energy: larger is smoother and less noisy</source>
        <translation>正则化（相对于通道平均能量）：越大越平滑、噪声越小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Regularisation α:</source>
        <translation>正则化 α：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Grey world</source>
        <translation>灰度世界</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>White patch</source>
        <translation>白块</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Reference area</source>
        <translation>参考区域</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>None (equal-energy light)</source>
        <translation>无（等能光源）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>White balance:</source>
        <translation>白平衡：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pick reference area</source>
        <translation>选择参考区域</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>This percentile of max(R, G, B) is shown as full brightness</source>
        <translation>max(R, G, B) 的该百分位数显示为满亮度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exposure:</source>
        <translation>曝光：</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>ENVI (.hdr + .img)</source>
        <translation>ENVI (.hdr + .img)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>MATLAB (.mat)</source>
        <translation>MATLAB (.mat)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Multi-page TIFF</source>
        <translation>多页 TIFF</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The standard exchange format for hyperspectral cubes: ENVI, Spectral Python, MATLAB hypercube, QGIS / GDAL</source>
        <translation>高光谱数据立方体的标准交换格式：ENVI、Spectral Python、MATLAB hypercube、QGIS / GDAL</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>32-bit float</source>
        <translation>32 位浮点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>16-bit unsigned</source>
        <translation>16 位无符号</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rounded and clipped to 0 … 65535: for raw counts only</source>
        <translation>四舍五入并截取到 0 … 65535：仅适用于原始计数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unprocessed cube</source>
        <translation>未处理的数据立方体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save the cube as extracted, without the processing steps</source>
        <translation>保存提取后的数据立方体，不含处理步骤</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save cube…</source>
        <translation>保存数据立方体…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save view (.png)…</source>
        <translation>保存视图 (.png)…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save spectra (.csv)…</source>
        <translation>保存光谱 (.csv)…</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Click a pixel or drag an area to keep its spectrum. The wheel zooms; drag with the middle button or with Ctrl held to pan. Right-click a box&apos;s label to remove it.</source>
        <translation>单击像素或拖出区域以保留其光谱。滚轮缩放；按住鼠标中键或按住 Ctrl 拖动可平移。右键单击方框的标签可将其删除。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>1 view</source>
        <translation>1 个视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>2 views</source>
        <translation>2 个视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>4 views</source>
        <translation>4 个视图</translation>
    </message>
    <message>
        <location line="+219"/>
        <source>Pixels further than this angle from every drawn area are left unmatched</source>
        <translation>与所有绘制区域的夹角都大于此值的像素不进行匹配</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Shade by similarity</source>
        <translation>按相似度明暗</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Full colour where the spectrum is the same as the area&apos;s, darker the more it differs</source>
        <translation>光谱与区域相同处为全色，差异越大越暗</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmatched: image</source>
        <translation>未匹配：显示图像</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Unmatched: black</source>
        <translation>未匹配：黑色</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max. angle:</source>
        <translation>最大夹角：</translation>
    </message>
    <message>
        <location line="+978"/>
        <source>Raw mosaic</source>
        <translation>原始马赛克</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Band</source>
        <translation>波段</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>All bands</source>
        <translation>全部波段</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Colour (sRGB)</source>
        <translation>彩色 (sRGB)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>False colour</source>
        <translation>伪彩色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Principal components</source>
        <translation>主成分</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Spectral index</source>
        <translation>光谱指数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Similar areas (spectral angle)</source>
        <translation>相似区域（光谱角）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Classes (k-means)</source>
        <translation>类别（k 均值）</translation>
    </message>
    <message>
        <location line="+303"/>
        <source>Draw areas with the spectrum tool: every pixel takes the colour of the area it resembles most.</source>
        <translation>请用光谱工具绘制区域：每个像素将显示为与其最相似区域的颜色。</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>unmatched %1 %</source>
        <translation>未匹配 %1 %</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>A/B compares the processed cube with the unprocessed one: turn on a processing step first</source>
        <translation>A/B 用于对比处理后与未处理的数据立方体：请先开启一个处理步骤</translation>
    </message>
    <message>
        <location line="-1571"/>
        <source>View:</source>
        <translation>视图：</translation>
    </message>
    <message>
        <location line="-489"/>
        <source>Click a view to choose what it shows in View: and the options below the image</source>
        <translation>点击一个视图，即可在“视图：”和图像下方的选项中选择它显示的内容</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Refresh this view: draw it again from the current settings</source>
        <translation>刷新此视图：按当前设置重新绘制</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Reset this view: its default view mode, band and false-colour bands, fitted to the window</source>
        <translation>重置此视图：恢复其默认视图模式、波段和伪彩色波段，并适应窗口</translation>
    </message>
    <message>
        <location line="+77"/>
        <source>Spectra</source>
        <translation>光谱</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scores (PCA)</source>
        <translation>得分 (PCA)</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Correction</source>
        <translation>校正</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Settings…</source>
        <translation>设置…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Spectral response details, spectral correction, exposure times, sensor profile</source>
        <translation>光谱响应详情、光谱校正、曝光时间、传感器配置</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+2678"/>
        <source>No spectral response.</source>
        <translation>无光谱响应。</translation>
    </message>
    <message>
        <location line="-2676"/>
        <source>Spectral correction</source>
        <translation>光谱校正</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Turn the 16 overlapping channels into narrow bands at the channel peaks (details under Settings…). Needs the spectral response or a correction matrix.</source>
        <translation>将 16 个相互重叠的通道转换为通道峰值处的窄波段（详见“设置…”）。需要光谱响应或校正矩阵。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>White frame</source>
        <translation>白参考帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Dark frame</source>
        <translation>暗帧</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Optional: a capture of a white reference filling the view, for a per-pixel flat field (gives reflectance)</source>
        <translation>可选：白参考充满视野时拍摄的图像，用于逐像素平场校正（得到反射率）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Optional: a capture with the lens covered, subtracted pixel by pixel (replaces the black level)</source>
        <translation>可选：遮住镜头时拍摄的图像，逐像素扣除（代替黑电平）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>From file…</source>
        <translation>从文件…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open white reference frame</source>
        <translation>打开白参考帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Open dark frame</source>
        <translation>打开暗帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Images (*.tif *.tiff *.png *.mat);;All files (*)</source>
        <translation>图像 (*.tif *.tiff *.png *.mat);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>From workspace…</source>
        <translation>从工作区…</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+2702"/>
        <location line="+1"/>
        <location line="+12"/>
        <source>none</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="-2712"/>
        <source>Remove the white frame</source>
        <translation>移除白参考帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Remove the dark frame</source>
        <translation>移除暗帧</translation>
    </message>
    <message>
        <location line="+141"/>
        <source>Absorbance (−log₁₀ R)</source>
        <translation>吸光度 (−log₁₀ R)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Turn reflectance into absorbance, which is roughly proportional to concentration (Beer–Lambert). Use with reflectance or a white frame.</source>
        <translation>将反射率转换为吸光度，后者大致与浓度成正比（比尔-朗伯定律）。请与反射率或白参考帧一起使用。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>SNV</source>
        <translation>SNV</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>MSC</source>
        <translation>MSC</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unit length</source>
        <translation>单位长度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unit area</source>
        <translation>单位面积</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Maximum = 1</source>
        <translation>最大值 = 1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Standard normal variate: every spectrum minus its mean, divided by its standard deviation. Removes brightness and scatter differences (shading, surface).</source>
        <translation>标准正态变量变换：每条光谱减去其均值后除以其标准差。消除亮度和散射差异（阴影、表面）。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Multiplicative scatter correction: every spectrum fitted to the mean spectrum (offset + slope) and corrected by the fit.</source>
        <translation>多元散射校正：每条光谱对平均光谱进行拟合（偏移 + 斜率），并按拟合结果校正。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Every spectrum divided by its length: keeps only the shape (as the spectral angle).</source>
        <translation>每条光谱除以其长度：只保留形状（与光谱角相同）。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Every spectrum divided by the sum of its values.</source>
        <translation>每条光谱除以其数值之和。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Every spectrum divided by its largest value.</source>
        <translation>每条光谱除以其最大值。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Spectrum normalisation: removes differences in brightness, so spectra compare by shape</source>
        <translation>光谱归一化：消除亮度差异，使光谱按形状比较</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Normalise:</source>
        <translation>归一化：</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>No smoothing</source>
        <translation>不平滑</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Smooth</source>
        <translation>平滑</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>1st derivative</source>
        <translation>一阶导数</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>2nd derivative</source>
        <translation>二阶导数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Savitzky–Golay filter along the spectrum: smoothing, or a derivative, which removes offsets (1st) and slopes (2nd) and sharpens overlapping features. With 16 bands, keep the window small.</source>
        <translation>沿光谱方向的 Savitzky–Golay 滤波：平滑或求导；导数可消除偏移（一阶）和斜率（二阶），并使重叠特征更清晰。只有 16 个波段时，窗口宜小。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>window </source>
        <translation>窗口 </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Number of bands in the window (odd)</source>
        <translation>窗口内的波段数（奇数）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>order </source>
        <translation>阶数 </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Order of the polynomial fitted in the window</source>
        <translation>窗口内拟合多项式的阶数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Savitzky–Golay:</source>
        <translation>Savitzky–Golay：</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Order: reflectance → MNF → guided → anti-aliasing → absorbance → normalise → Savitzky–Golay → upscale.</source>
        <translation>顺序：反射率 → MNF → 引导滤波 → 抗混叠 → 吸光度 → 归一化 → Savitzky–Golay → 放大。</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Needs the spectral response (Correction panel).</source>
        <translation>需要光谱响应（校正面板）。</translation>
    </message>
    <message>
        <location line="+82"/>
        <source>Pan and zoom</source>
        <translation>平移和缩放</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Spectrum: click a pixel or drag an area</source>
        <translation>光谱：单击像素或拖出区域</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pick colour reference area</source>
        <translation>选择白平衡参考区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pick reference for similar areas</source>
        <translation>选择相似区域的参考</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Mouse:</source>
        <translation>鼠标：</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Show the cube in two or four views side by side. Click a view to make it the active one: View: and the options below the image then set what it shows. Zoom, pan and the mouse pointer follow in all of them.</source>
        <translation>以两个或四个并排视图显示数据立方体。点击一个视图使其成为当前视图：“视图：”和图像下方的选项随后设置它显示的内容。缩放、平移和鼠标指针在所有视图中同步。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Make the cube again from the source with the current settings (extraction, correction, processing) and draw every view again</source>
        <translation>按当前设置（提取、校正、处理）从源图像重新生成数据立方体，并重新绘制所有视图</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+9"/>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Turn all processing off, set colour reconstruction back to its defaults and every view back to its default mode, band and zoom. The source, extraction settings, spectral response, correction, reference frames and drawn areas are kept.</source>
        <translation>关闭所有处理，将颜色重建恢复为默认设置，并将每个视图恢复为默认模式、波段和缩放。源图像、提取设置、光谱响应、校正、参考帧和绘制的区域保持不变。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Turn all processing off and set colour reconstruction and every view back to their defaults?</source>
        <translation>关闭所有处理，并将颜色重建和所有视图恢复为默认设置？</translation>
    </message>
    <message>
        <location line="+8"/>
        <location line="+1515"/>
        <source>Compare with the unprocessed cube: drag the divider across the image</source>
        <translation>与未处理的数据立方体对比：在图像上拖动分隔线</translation>
    </message>
    <message>
        <location line="-1486"/>
        <source>The raw sensor image, before extraction.</source>
        <translation>提取前的原始传感器图像。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Band:</source>
        <translation>波段：</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>All bands with one shared contrast, channel 1 at the top left.</source>
        <translation>全部波段使用同一对比度，通道 1 在左上角。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Spectrum per pixel from the spectral response (Wiener estimate) → CIE 1931 XYZ → white balance → sRGB, as hsi_reconstruct.m. Without a spectral response, a natural false-colour picture is shown.</source>
        <translation>由光谱响应估计每个像素的光谱（维纳估计）→ CIE 1931 XYZ → 白平衡 → sRGB，与 hsi_reconstruct.m 相同。未加载光谱响应时显示接近自然色的伪彩色图像。</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Natural</source>
        <translation>自然色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Most contrast</source>
        <translation>最大对比</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The three least correlated bands</source>
        <translation>相关性最低的三个波段</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Bands as b1 … b16, or R&lt;nm&gt; for the band nearest a wavelength (needs the spectral response); + − × / ^ ( ), sqrt, abs, log, exp</source>
        <translation>波段写作 b1 … b16，或用 R&lt;nm&gt; 表示最接近该波长的波段（需要光谱响应）；+ − × / ^ ( )、sqrt、abs、log、exp</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Grey</source>
        <translation>灰度</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Diverging</source>
        <translation>发散</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Pixels whose spectrum is within this angle of the reference are marked</source>
        <translation>光谱与参考之间的夹角在此范围内的像素将被标记</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Marked on the image</source>
        <translation>在图像上标记</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Angle map</source>
        <translation>角度图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pick reference</source>
        <translation>选择参考</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1480"/>
        <source>Choose a reference: click a pixel or drag an area.</source>
        <translation>选择参考：单击像素或拖出区域。</translation>
    </message>
    <message>
        <location line="-1478"/>
        <source>Threshold:</source>
        <translation>阈值：</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>k = </source>
        <translation>k = </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>By spectral shape (ignore brightness)</source>
        <translation>按光谱形状（忽略亮度）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Find classes</source>
        <translation>查找类别</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Wavelength</source>
        <translation>波长</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>x axis in nm (needs the spectral response), otherwise band numbers</source>
        <translation>x 轴以 nm 为单位（需要光谱响应），否则为波段编号</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Normalise</source>
        <translation>归一化</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Divide each curve by its maximum, to compare shapes</source>
        <translation>将每条曲线除以其最大值，以比较形状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Class means</source>
        <translation>类别均值</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Kept points and areas:</source>
        <translation>保留的点和区域：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Remove</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove all</source>
        <translation>全部删除</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>x:</source>
        <translation>x：</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>y:</source>
        <translation>y：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Load an image to see its principal component scores.</source>
        <translation>加载图像以查看其主成分得分。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Every pixel as a point (brighter: more pixels). Drag a loop around a group of points to select those pixels in the image; right-click or Esc removes the selection. The kept areas show in their colours.</source>
        <translation>每个像素为一个点（越亮表示像素越多）。拖动画圈围住一组点即可在图像中选中这些像素；右键或 Esc 取消选择。保留的区域以其颜色显示。</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+1831"/>
        <source>No selection.</source>
        <translation>未选择。</translation>
    </message>
    <message>
        <location line="-1830"/>
        <source>Clear selection</source>
        <translation>清除选择</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Keep</source>
        <translation>保留</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Keep the mean spectrum of the selection in the list of spectra</source>
        <translation>将所选区域的平均光谱保留在光谱列表中</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Loadings (what each component weighs):</source>
        <translation>载荷（各主成分的权重）：</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Loading</source>
        <translation>载荷</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Open a raw mosaic, a cube or a spectral response</source>
        <translation>打开原始马赛克、数据立方体或光谱响应</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hyperspectral files (*.tif *.tiff *.png *.jpg *.jpeg *.mat *.hdr *.img *.csv *.txt);;All files (*)</source>
        <translation>高光谱文件 (*.tif *.tiff *.png *.jpg *.jpeg *.mat *.hdr *.img *.csv *.txt);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+22"/>
        <location line="+25"/>
        <source>Cube from %1 (%2 × %3 × %4)</source>
        <translation>来自 %1 的数据立方体（%2 × %3 × %4）</translation>
    </message>
    <message>
        <location line="+8"/>
        <location line="+1989"/>
        <source>%1 holds no image.</source>
        <translation>%1 中没有图像。</translation>
    </message>
    <message>
        <location line="-1984"/>
        <location line="+35"/>
        <source>%1 · %2 · %3</source>
        <translation>%1 · %2 · %3</translation>
    </message>
    <message>
        <location line="-33"/>
        <source> · frame 1 of %1 (use a tab and “Import from workspace” for the mean)</source>
        <translation> · 第 1 帧，共 %1 帧（如需均值，请在标签页中加载后“从工作区导入”）</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>%1 · %2 %3</source>
        <translation>%1 · %2 %3</translation>
    </message>
    <message>
        <location line="+926"/>
        <source>Processing and views reset to their defaults.</source>
        <translation>处理和视图已恢复为默认设置。</translation>
    </message>
    <message>
        <location line="+954"/>
        <source>%1 · %2 channels</source>
        <translation>%1 · %2 个通道</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1 · colour matrix only</source>
        <translation>%1 · 仅颜色矩阵</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Correction: %1 % residual, noise × %2</source>
        <translation>校正：残差 %1 %，噪声 × %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Correction: %1, noise × %2</source>
        <translation>校正：%1，噪声 × %2</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>%1
%2</source>
        <translation>%1
%2</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>%1
%2
Filter peaks (nm): %3
Peak widths, FWHM (nm): %4</source>
        <translation>%1
%2
滤光片峰值 (nm)：%3
峰宽，半高宽 (nm)：%4</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Dark: %1</source>
        <translation>暗帧：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>White: %1</source>
        <translation>白参考帧：%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The frames apply to raw mosaics, not to a cube opened from a file.</source>
        <translation>参考帧作用于原始马赛克图像，不作用于从文件打开的数据立方体。</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Not used: the dark frame is subtracted instead.</source>
        <translation>未使用：改为扣除暗帧。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The %1 frame is %2 × %3, the image %4 × %5: they must be the same size.</source>
        <translation>%1 为 %2 × %3，图像为 %4 × %5：两者尺寸必须相同。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>white</source>
        <translation>白参考帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>dark</source>
        <translation>暗帧</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Reference frames apply to raw mosaics; this cube was opened from a file.</source>
        <translation>参考帧作用于原始马赛克图像；此数据立方体是从文件打开的。</translation>
    </message>
    <message>
        <location line="+30"/>
        <source> (mean of %1 frames)</source>
        <translation>（%1 帧的平均）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>White reference frame from workspace</source>
        <translation>从工作区选择白参考帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Dark frame from workspace</source>
        <translation>从工作区选择暗帧</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Save sensor profile</source>
        <translation>保存传感器配置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Sensor profiles (*.iatsensor)</source>
        <translation>传感器配置 (*.iatsensor)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Load sensor profile</source>
        <translation>加载传感器配置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sensor profiles (*.iatsensor);;All files (*)</source>
        <translation>传感器配置 (*.iatsensor);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Saved sensor profile %1</source>
        <translation>已保存传感器配置 %1</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Sensor profile loaded: %1</source>
        <translation>已加载传感器配置：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sensor profile loaded; not found: %1</source>
        <translation>已加载传感器配置；未找到：%1</translation>
    </message>
    <message>
        <location line="-2012"/>
        <source>White area: rows %1–%2, columns %3–%4</source>
        <translation>白参考区域：第 %1–%2 行，第 %3–%4 列</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Layout: %1</source>
        <translation>排列：%1</translation>
    </message>
    <message>
        <location line="+90"/>
        <source>The mosaic layout is not valid: every channel 1–16 must appear exactly once.</source>
        <translation>马赛克排列无效：通道 1–16 必须各出现一次。</translation>
    </message>
    <message>
        <location line="+1356"/>
        <source>Value (DN)</source>
        <translation>数值 (DN)</translation>
    </message>
    <message>
        <location line="-1169"/>
        <source>No cube yet.</source>
        <translation>尚无数据立方体。</translation>
    </message>
    <message>
        <location line="-719"/>
        <source>About 610, 550 and 490 nm</source>
        <translation>约 610、550 和 490 nm</translation>
    </message>
    <message>
        <location line="+722"/>
        <source>Cube: %1 × %2 × %3 bands</source>
        <translation>数据立方体：%1 × %2 × %3 个波段</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>, %1 × %2 after processing</source>
        <translation>，处理后 %1 × %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>, %1–%2 nm</source>
        <translation>，%1–%2 nm</translation>
    </message>
    <message>
        <location line="+44"/>
        <location line="+6"/>
        <source>Band %1</source>
        <translation>波段 %1</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Load spectral response</source>
        <translation>加载光谱响应</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Spectral response (*.mat *.csv *.txt);;All files (*)</source>
        <translation>光谱响应 (*.mat *.csv *.txt);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Spectral response loaded: %1</source>
        <translation>已加载光谱响应：%1</translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Choose a reference area for the white balance (Pick reference area); grey world is used until then.</source>
        <translation>请为白平衡选择参考区域（选择参考区域）；在此之前使用灰度世界。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>The colour matrix could not be computed (singular matrix or a black reference area).</source>
        <translation>无法计算颜色矩阵（矩阵奇异或参考区域为黑色）。</translation>
    </message>
    <message>
        <location line="+141"/>
        <source>Click a pixel or drag an area to keep its spectrum.</source>
        <translation>单击像素或拖出区域以保留其光谱。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drag a rectangle over the white reference.</source>
        <translation>在白参考物上拖出一个矩形。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drag a rectangle over a white or grey area for the white balance.</source>
        <translation>在白色或灰色区域上拖出矩形作为白平衡参考。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Click a pixel or drag an area: similar areas will be marked.</source>
        <translation>单击像素或拖出区域：将标记出相似区域。</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>Match to drawn areas</source>
        <translation>匹配绘制区域</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Every pixel in the colour of the drawn area whose spectrum it resembles most (spectral angle mapper classification), darker the less it resembles it</source>
        <translation>每个像素显示为光谱与其最相似的绘制区域的颜色（光谱角制图分类），相似度越低越暗</translation>
    </message>
    <message>
        <location line="+234"/>
        <source>Red, green, blue = first three principal components. Variance: %1</source>
        <translation>红、绿、蓝 = 前三个主成分。方差：%1</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Choose a preset or type a formula, e.g. (b7 - b11) / (b7 + b11).</source>
        <translation>请选择预设或输入公式，例如 (b7 - b11) / (b7 + b11)。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 · display range %2 … %3 (2–98 %)</source>
        <translation>%1 · 显示范围 %2 … %3（2–98 %）</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Reference: %1 · %2 pixels (%3 %) within %4°</source>
        <translation>参考：%1 · %2 个像素（%3 %）在 %4° 以内</translation>
    </message>
    <message>
        <location line="+117"/>
        <source>White</source>
        <translation>白参考</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>WB</source>
        <translation>白平衡</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+344"/>
        <source>Reference</source>
        <translation>参考</translation>
    </message>
    <message>
        <location line="-202"/>
        <source>Pixel (%1, %2)</source>
        <translation>像素 (%1, %2)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Row %1, column %2</source>
        <translation>第 %1 行，第 %2 列</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Sensor row %1, column %2: %3 DN (channel %4)</source>
        <translation>传感器第 %1 行，第 %2 列：%3 DN（通道 %4）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · band %1: %2</source>
        <translation> · 波段 %1：%2</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> · index: %1</source>
        <translation> · 指数：%1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source> · angle to reference: %1°</source>
        <translation> · 与参考的夹角：%1°</translation>
    </message>
    <message>
        <location line="+2"/>
        <source> · class %1</source>
        <translation> · 类别 %1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · matches %1 (%2°)</source>
        <translation> · 匹配 %1（%2°）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source> · no match within %1°</source>
        <translation> · %1° 内无匹配</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>rows %1–%2, columns %3–%4</source>
        <translation>第 %1–%2 行，第 %3–%4 列</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Point %1</source>
        <translation>点 %1</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+41"/>
        <source>Area %1</source>
        <translation>区域 %1</translation>
    </message>
    <message>
        <location line="-20"/>
        <source>pixel (%1, %2)</source>
        <translation>像素 (%1, %2)</translation>
    </message>
    <message>
        <location line="+78"/>
        <source>Finding %1 classes …</source>
        <translation>正在查找 %1 个类别…</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>%1 · row %2, column %3</source>
        <translation>%1 · 第 %2 行，第 %3 列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 · %2 × %3 px</source>
        <translation>%1 · %2 × %3 像素</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Similar area (%1 px)</source>
        <translation>相似区域（%1 像素）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Class %1</source>
        <translation>类别 %1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Selection (%1 px)</source>
        <translation>所选区域（%1 像素）</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Absorbance</source>
        <translation>吸光度</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Normalised value</source>
        <translation>归一化值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Derivative</source>
        <translation>导数</translation>
    </message>
    <message>
        <location line="+24"/>
        <location line="+1"/>
        <location line="+24"/>
        <source>PC%1</source>
        <translation>PC%1</translation>
    </message>
    <message>
        <location line="-16"/>
        <source>PC%1 (%2 %)</source>
        <translation>PC%1 (%2 %)</translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Selection %1 (%2 px)</source>
        <translation>所选区域 %1（%2 像素）</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>%1 pixels selected (%2 %)</source>
        <translation>已选择 %1 个像素（%2 %）</translation>
    </message>
    <message>
        <location line="+33"/>
        <location line="+75"/>
        <source>There is no cube to save.</source>
        <translation>没有可保存的数据立方体。</translation>
    </message>
    <message>
        <location line="-66"/>
        <location line="+29"/>
        <location line="+30"/>
        <source>Saved %1</source>
        <translation>已保存 %1</translation>
    </message>
    <message>
        <location line="-30"/>
        <source>Could not save %1</source>
        <translation>无法保存 %1</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>There are no spectra to save: keep a point or an area with the spectrum tool first.</source>
        <translation>没有可保存的光谱：请先用光谱工具保留一个点或区域。</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>ENVI header (*.hdr)</source>
        <translation>ENVI 头文件 (*.hdr)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>MAT-files (*.mat)</source>
        <translation>MAT 文件 (*.mat)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>TIFF files (*.tif *.tiff)</source>
        <translation>TIFF 文件 (*.tif *.tiff)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save cube</source>
        <translation>保存数据立方体</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Save view</source>
        <translation>保存视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>PNG images (*.png)</source>
        <translation>PNG 图像 (*.png)</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Save spectra</source>
        <translation>保存光谱</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>CSV files (*.csv)</source>
        <translation>CSV 文件 (*.csv)</translation>
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
        <translation>此处请加载一个文件。多张图像可以在场景帧和暗场帧面板中合并。</translation>
    </message>
</context>
<context>
    <name>iat::ImagePanel</name>
    <message>
        <location filename="../src/ui/ImagePanel.cpp" line="+146"/>
        <source>Auto calibration off</source>
        <translation>自动校准已关闭</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>This panel shows an image loaded from a file, so it is not calculated from
the light and dark frames.

Click to remove the loaded image and turn automatic calibration
(light image − dark image) back on.</source>
        <translation>此面板显示的是从文件加载的图像，因此不是由场景帧和暗场帧
计算得到的。

点击可移除已加载的图像，并重新开启自动校准
（场景图像 − 暗场图像）。</translation>
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
        <location line="+27"/>
        <source>Processing…</source>
        <translation>图像处理…</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Load…</source>
        <translation>加载…</translation>
    </message>
    <message>
        <location line="+5"/>
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
        <translation>场景图像 − 暗场图像</translation>
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
        <location line="+814"/>
        <source>Mean [DN]</source>
        <translation>均值 [DN]</translation>
    </message>
    <message>
        <location line="-813"/>
        <location line="+813"/>
        <source>Std [DN]</source>
        <translation>标准差 [DN]</translation>
    </message>
    <message>
        <location line="-812"/>
        <location line="+812"/>
        <source>Noise [DN]</source>
        <translation>噪声 [DN]</translation>
    </message>
    <message>
        <location line="-795"/>
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
        <location line="+9"/>
        <source>Image (.png)</source>
        <translation>图像 (.png)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Save the image as displayed (current contrast, full resolution) to a PNG file</source>
        <translation>将当前显示的图像（当前对比度、完整分辨率）保存为 PNG 文件</translation>
    </message>
    <message>
        <location line="+41"/>
        <location line="+457"/>
        <source>Image processing: %1</source>
        <translation>图像处理：%1</translation>
    </message>
    <message>
        <location line="-440"/>
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
        <location line="+40"/>
        <source>The image has no finite values (all NaN or Inf).</source>
        <translation>图像中没有有限值（全部为 NaN 或 Inf）。</translation>
    </message>
    <message>
        <location line="+105"/>
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
        <translation>场景图像 − 暗场图像（当前显示的图像）</translation>
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
        <location line="+95"/>
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
        <location line="+3"/>
        <source> · processed</source>
        <translation> · 已处理</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1

Image processing is on, so the histogram, Mean, Std, Min and Max describe the processed image%2:
%3

Noise is still the temporal noise of the unprocessed frames.</source>
        <translation>%1

图像处理已开启，因此直方图、均值、标准差、最小值和最大值描述的是处理后的图像%2：
%3

噪声仍是未处理帧的时域噪声。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source> (display values, 0 to 255)</source>
        <translation>（显示值，0 到 255）</translation>
    </message>
    <message>
        <location line="+12"/>
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
        <translation>全部 %1 个场景帧的时域噪声

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

从每个场景帧中减去同一幅图像 D 不会改变
时域方差，因此该值等于场景帧的噪声。</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Hover to read pixel values · wheel to zoom · double-click to fit · right-click to copy</source>
        <translation>悬停读取像素值 · 滚轮缩放 · 双击适应窗口 · 右键复制</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Row %1, Col %2:  %3  (unprocessed %4)</source>
        <translation>行 %1，列 %2：  %3  （未处理 %4）</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Row %1, Col %2:  %3</source>
        <translation>行 %1，列 %2：  %3</translation>
    </message>
    <message>
        <location line="+91"/>
        <source>Image processing for this panel: defect pixel and line correction, denoising,
dynamic range compression, tone mapping, detail enhancement and sharpening,
as a chain of stages that can each be turned on or bypassed.</source>
        <translation>本面板的图像处理：坏点与坏行/坏列校正、降噪、
动态范围压缩、色调映射、细节增强和锐化，
以一串可逐个开启或旁路的处理阶段呈现。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>

Showing the original image. Set up: %1</source>
        <translation>

正在显示原始图像。已设置：%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>

On: %1</source>
        <translation>

已开启：%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>A/B comparison: shows a divider on the image. Left of it (A) is the unprocessed
image, right of it (B) the processed one. Drag the divider across the image;
double-click it to put it back in the middle.</source>
        <translation>A/B 对比：在图像上显示一条分隔线。分隔线左侧 (A) 为未处理的图像，
右侧 (B) 为处理后的图像。可在图像上左右拖动分隔线；
双击分隔线可使其回到中间。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>A/B comparison of the unprocessed and the processed image.
Turn on a stage under Processing… first.</source>
        <translation>未处理图像与处理后图像的 A/B 对比。
请先在“图像处理…”中开启某个阶段。</translation>
    </message>
    <message>
        <location line="+74"/>
        <source>Image processing failed (not enough memory?). Showing the unprocessed image.</source>
        <translation>图像处理失败（内存不足？）。显示未处理的图像。</translation>
    </message>
    <message>
        <location line="+127"/>
        <source>Display range %1 to %2 · Min %3 · Max %4 · %5×%6 px</source>
        <translation>显示范围 %1 至 %2 · 最小 %3 · 最大 %4 · %5×%6 像素</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · noise over %1 frames</source>
        <translation> · %1 帧的噪声</translation>
    </message>
    <message>
        <location line="+2"/>
        <source> · processed: %1</source>
        <translation> · 已处理：%1</translation>
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
    <message>
        <location line="+6"/>
        <source>Image processing…</source>
        <translation>图像处理…</translation>
    </message>
</context>
<context>
    <name>iat::ImageView</name>
    <message>
        <location filename="../src/ui/ImageView.cpp" line="+451"/>
        <source>Right-click to remove</source>
        <translation>右键单击以删除</translation>
    </message>
</context>
<context>
    <name>iat::ImportImageDialog</name>
    <message>
        <location filename="../src/ui/ImportImageDialog.cpp" line="+18"/>
        <source>Import from workspace</source>
        <translation>从工作区导入</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Choose a tab, then the image to use as the raw 4 × 4 mosaic. The values are taken as they are in that tab, before any image processing.</source>
        <translation>选择一个标签页，再选择用作原始 4 × 4 马赛克的图像。数值按该标签页中的原样获取，不经过任何图像处理。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Tab:</source>
        <translation>标签页：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Image:</source>
        <translation>图像：</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Import</source>
        <translation>导入</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>No tab has an image yet.

Load an image in single image mode, or light and dark frames in three-panel mode, then import it here.</source>
        <translation>还没有标签页包含图像。

请在单图像模式下加载图像，或在三面板模式下加载场景帧和暗场帧，然后在此导入。</translation>
    </message>
</context>
<context>
    <name>iat::MainWindow</name>
    <message>
        <location filename="../src/ui/MainWindow.cpp" line="+26"/>
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
        <translation>将一个 .mat 文件，或一张或多张 TIFF、PNG、JPG 图像拖放到面板上。如需场景帧、暗场帧和校准帧，请使用右上角的切换开关。</translation>
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
        <location line="+85"/>
        <source>Open &amp;light frames…</source>
        <translation>打开场景帧(&amp;L)…</translation>
    </message>
    <message>
        <location line="-78"/>
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
        <location line="+14"/>
        <source>Session %1</source>
        <translation>会话 %1</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Open &amp;image…</source>
        <translation>打开图像(&amp;I)…</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Open raw &amp;mosaic or cube…</source>
        <translation>打开原始马赛克或数据立方体(&amp;M)…</translation>
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
        <location filename="../src/ui/ModeSwitch.cpp" line="+85"/>
        <source>Panel layout</source>
        <translation>面板布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Single image; three panels: light, dark and calibrated frames; or hyperspectral</source>
        <translation>单图像；三个面板：场景帧、暗场帧和校准帧；或高光谱</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Single image: one panel</source>
        <translation>单图像：一个面板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Three panels: light frames, dark frames and calibrated frames</source>
        <translation>三个面板：场景帧、暗场帧和校准帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hyperspectral: raw 4 × 4 mosaic → 16-band cube, colour and spectra</source>
        <translation>高光谱：原始 4 × 4 马赛克 → 16 波段数据立方体、颜色与光谱</translation>
    </message>
</context>
<context>
    <name>iat::MosaicLayoutDialog</name>
    <message>
        <location filename="../src/ui/MosaicLayoutDialog.cpp" line="+20"/>
        <source>Mosaic layout</source>
        <translation>马赛克排列</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Channel at each position of the 4 × 4 super-pixel (as seen on the sensor):</source>
        <translation>4 × 4 超像素中每个位置的通道（按传感器上的方向）：</translation>
    </message>
    <message>
        <location line="+91"/>
        <source>Row by row (vertical)</source>
        <translation>逐行（纵向）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Serpentine</source>
        <translation>蛇形</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Serpentine (vertical)</source>
        <translation>蛇形（纵向）</translation>
    </message>
    <message>
        <location line="-6"/>
        <source>Row by row</source>
        <translation>逐行</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Every channel 1–16 must appear exactly once. Missing: %1.</source>
        <translation>通道 1–16 必须各出现一次。缺少：%1。</translation>
    </message>
</context>
<context>
    <name>iat::PipelineDialog</name>
    <message>
        <location filename="../src/ui/PipelineDialog.cpp" line="+308"/>
        <source>Image processing</source>
        <translation>图像处理</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The image passes through the stages from left to right. Use the switch on a stage to turn it on or to bypass it; click a stage to choose its algorithm and settings.</source>
        <translation>图像从左到右依次通过各个阶段。用阶段上的开关开启或旁路该阶段；点击阶段可选择其算法和参数。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Input</source>
        <translation>输入</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Output</source>
        <translation>输出</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Enabled</source>
        <translation>启用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Turn this stage on. When it is off, the image passes through unchanged (bypass).</source>
        <translation>开启此阶段。关闭时图像原样通过（旁路）。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Algorithm</source>
        <translation>算法</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Presets</source>
        <translation>预设</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ready-made settings for common situations. A preset replaces all settings.</source>
        <translation>针对常见情况的现成设置。预设会替换全部设置。</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Reset stage</source>
        <translation>重置阶段</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Default settings for the selected stage, and bypass it</source>
        <translation>将所选阶段恢复为默认设置并旁路</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Reset all</source>
        <translation>全部重置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Default settings for every stage, all bypassed: the image is shown unprocessed</source>
        <translation>所有阶段恢复默认设置并全部旁路：显示未处理的图像</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Show original</source>
        <translation>显示原图</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show the unprocessed image, keeping all settings, to compare before and after</source>
        <translation>显示未处理的图像并保留全部设置，便于对比处理前后</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>A/B comparison: shows a divider on the panel&apos;s image. Left of it (A) is the unprocessed
image, right of it (B) the processed one. Drag the divider across the image.</source>
        <translation>A/B 对比：在面板的图像上显示一条分隔线。分隔线左侧 (A) 为未处理的图像，
右侧 (B) 为处理后的图像。可在图像上左右拖动分隔线。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+150"/>
        <source>Bypassed</source>
        <translation>已旁路</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Showing the original image</source>
        <translation>正在显示原始图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Every stage is bypassed: the image is shown unprocessed</source>
        <translation>所有阶段均已旁路：显示未处理的图像</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Processing …</source>
        <translation>正在处理…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Processed in %1 ms</source>
        <translation>处理用时 %1 ms</translation>
    </message>
</context>
<context>
    <name>iat::SessionTab</name>
    <message>
        <location filename="../src/ui/SessionTab.cpp" line="+164"/>
        <location line="+311"/>
        <source>image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="-311"/>
        <location line="+448"/>
        <source>light frames</source>
        <translation>场景帧</translation>
    </message>
    <message>
        <location line="-447"/>
        <location line="+450"/>
        <source>dark frames</source>
        <translation>暗场帧</translation>
    </message>
    <message>
        <location line="-447"/>
        <source>calibrated frames</source>
        <translation>校准帧</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>mean of %1 frames</source>
        <translation>%1 帧的均值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>frame %1 of %2</source>
        <translation>第 %1 帧，共 %2 帧</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Light frames</source>
        <translation>场景帧</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Dark frames</source>
        <translation>暗场帧</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>light − dark · %1 × %2</source>
        <translation>场景 − 暗场 · %1 × %2</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Calibrated frames</source>
        <translation>校准帧</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Single image</source>
        <translation>单图像</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Three panels: light frames, dark frames and calibrated frames (light − dark)</source>
        <translation>三个面板：场景帧、暗场帧和校准帧（场景 − 暗场）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hyperspectral: raw 4 × 4 mosaic → 16-band cube</source>
        <translation>高光谱：原始 4 × 4 马赛克 → 16 波段数据立方体</translation>
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
        <translation>打开场景帧：一个 .mat 文件，或一张或多张图像</translation>
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
        <translation>自动校准已开启：场景图像 − 暗场图像</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Mean of %1</source>
        <translation>%1的均值</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>light frame</source>
        <translation>场景帧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Light frame %1 of %2</source>
        <translation>场景帧 %1/%2</translation>
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

请加载场景帧以计算
场景图像 − 暗场图像。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Light frames ready.

Now load dark frames to compute
light image − dark image.</source>
        <translation>场景帧已就绪。

请加载暗场帧以计算
场景图像 − 暗场图像。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Waiting for the mean light and dark images …</source>
        <translation>正在等待场景和暗场的均值图像…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Waiting for the mean dark image …</source>
        <translation>正在等待暗场均值图像…</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Waiting for the mean light image …</source>
        <translation>正在等待场景均值图像…</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+10"/>
        <source>Light image − dark image</source>
        <translation>场景图像 − 暗场图像</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>The sizes do not match:

light image  %1 × %2
dark image   %3 × %4

Load files with the same X × Y size.</source>
        <translation>尺寸不一致：

场景图像  %1 × %2
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
        <location line="+36"/>
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
        <location line="+14"/>
        <source>Follow the system</source>
        <translation>跟随系统</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Light</source>
        <translation>浅色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Light or dark colours for the whole app. It changes as soon as you click OK.</source>
        <translation>整个应用使用浅色或深色配色。点击“确定”后立即生效。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>A new language is used after the app restarts.</source>
        <translation>新的语言在程序重新启动后生效。</translation>
    </message>
    <message>
        <location line="+52"/>
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
    <name>iat::SpectrumPlot</name>
    <message>
        <location filename="../src/ui/SpectrumPlot.cpp" line="+39"/>
        <source>Value</source>
        <translation>数值</translation>
    </message>
    <message>
        <location line="+89"/>
        <source>Hover over the image for the spectrum of a pixel;
click or drag with the spectrum tool to keep one.</source>
        <translation>将鼠标悬停在图像上查看像素光谱；
用光谱工具单击或拖动以保留光谱。</translation>
    </message>
    <message>
        <location line="+70"/>
        <source>Wavelength (nm)</source>
        <translation>波长 (nm)</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Band</source>
        <translation>波段</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Normalised</source>
        <translation>归一化</translation>
    </message>
    <message>
        <location line="+82"/>
        <source>Band %1 · %2 nm</source>
        <translation>波段 %1 · %2 nm</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Band %1</source>
        <translation>波段 %1</translation>
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
    <name>iat::hsi</name>
    <message>
        <location filename="../src/core/Hsi.cpp" line="+208"/>
        <source>Band %1</source>
        <translation>波段 %1</translation>
    </message>
    <message>
        <location line="+68"/>
        <source>The mosaic layout must contain every channel 1–16 exactly once.</source>
        <translation>马赛克排列必须恰好包含通道 1–16 各一次。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>The image is too small for a 4 × 4 mosaic.</source>
        <translation>图像太小，不足一个 4 × 4 马赛克。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cropped from %1 × %2 to %3 × %4 (a multiple of 4).</source>
        <translation>已从 %1 × %2 裁剪为 %3 × %4（4 的倍数）。</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Values exceed %1: treated as MSB-aligned and shifted down.</source>
        <translation>数值超过 %1：视为高位对齐数据并已右移。</translation>
    </message>
    <message>
        <location filename="../src/core/HsiAnalysis.cpp" line="+499"/>
        <source>Empty formula.</source>
        <translation>公式为空。</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+189"/>
        <source>Unexpected “%1” at position %2.</source>
        <translation>位置 %2 处出现意外的“%1”。</translation>
    </message>
    <message>
        <location line="-88"/>
        <source>The formula ends too early.</source>
        <translation>公式不完整。</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+74"/>
        <source>A closing parenthesis is missing.</source>
        <translation>缺少右括号。</translation>
    </message>
    <message>
        <location line="-52"/>
        <source>“%1” is not a number.</source>
        <translation>“%1”不是数字。</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>There is no band %1 (the cube has bands 1–%2).</source>
        <translation>没有波段 %1（数据立方体的波段为 1–%2）。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>R%1 needs the band wavelengths: load a spectral response (QE) file first, or use b1 … b%2.</source>
        <translation>R%1 需要波段波长：请先加载光谱响应（QE）文件，或使用 b1 … b%2。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>%1 nm is outside the sensor&apos;s range (%2–%3 nm).</source>
        <translation>%1 nm 超出传感器的范围（%2–%3 nm）。</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>%1 must be followed by “(”.</source>
        <translation>%1 后面必须跟“(”。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Unknown name “%1”. Use b1 … b%2 for bands, R&lt;nm&gt; for wavelengths.</source>
        <translation>未知名称“%1”。波段请用 b1 … b%2，波长请用 R&lt;nm&gt;。</translation>
    </message>
    <message>
        <location filename="../src/io/HsiCubeIo.cpp" line="+115"/>
        <location line="+230"/>
        <location line="+112"/>
        <source>There is no cube to save.</source>
        <translation>没有可保存的数据立方体。</translation>
    </message>
    <message>
        <location line="-265"/>
        <location line="+6"/>
        <location line="+337"/>
        <location line="+333"/>
        <location line="+99"/>
        <source>Could not write %1: %2</source>
        <translation>无法写入 %1：%2</translation>
    </message>
    <message>
        <location line="-754"/>
        <source>Could not open the ENVI header %1.</source>
        <translation>无法打开 ENVI 头文件 %1。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>%1 is not an ENVI header (it does not start with “ENVI”).</source>
        <translation>%1 不是 ENVI 头文件（不以“ENVI”开头）。</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>The ENVI header does not give the size (samples, lines, bands).</source>
        <translation>ENVI 头文件中没有尺寸信息（samples、lines、bands）。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>ENVI data type %1 is not supported (complex data?).</source>
        <translation>不支持 ENVI 数据类型 %1（复数数据？）。</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>The data file for %1 was not found (expected .img, .dat, .raw or no extension).</source>
        <translation>未找到 %1 的数据文件（应为 .img、.dat、.raw 或无扩展名）。</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+390"/>
        <location line="+130"/>
        <location line="+93"/>
        <source>Could not open %1: %2</source>
        <translation>无法打开 %1：%2</translation>
    </message>
    <message>
        <location line="-606"/>
        <source>%1 is too short: %2 bytes expected after the header offset, %3 found.</source>
        <translation>%1 太短：头偏移之后应有 %2 字节，实际 %3 字节。</translation>
    </message>
    <message>
        <location line="+138"/>
        <source>%1 has no 3-D array (rows × columns × bands).</source>
        <translation>%1 中没有三维数组（行 × 列 × 波段）。</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Variable “%1” used as the cube.</source>
        <translation>已将变量“%1”用作数据立方体。</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>The cube is too large for a classic TIFF file (4 GB). Save it as ENVI instead.</source>
        <translation>数据立方体超出经典 TIFF 文件的上限（4 GB），请改存为 ENVI。</translation>
    </message>
    <message>
        <location line="+149"/>
        <source>%1: %2 has %3 × %4 values, but there are %5 wavelengths.</source>
        <translation>%1：%2 有 %3 × %4 个值，但波长有 %5 个。</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>%1 channels, %2 wavelengths (%3–%4 nm), from %5 and %6.</source>
        <translation>%1 个通道，%2 个波长（%3–%4 nm），取自 %5 和 %6。</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Colour matrix reconRGB (3 × %1), used as it is: its white balance was fixed when it was made.</source>
        <translation>颜色矩阵 reconRGB（3 × %1）按原样使用：其白平衡在生成时已经确定。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 has neither a wavelength vector (lambda) with a QE matrix (QE_percent or QE) nor a 3 × N matrix reconRGB.</source>
        <translation>%1 中既没有波长向量（lambda）及 QE 矩阵（QE_percent 或 QE），也没有 3 × N 矩阵 reconRGB。</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>%1 has no header row with Band1 … BandN columns.</source>
        <translation>%1 中没有包含 Band1 … BandN 列的表头行。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>%1: the band columns must be Band1 … Band%2; Band%3 is missing.</source>
        <translation>%1：波段列应为 Band1 … Band%2；缺少 Band%3。</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>%1 has fewer than 3 rows of data under the header.</source>
        <translation>%1 的表头下数据少于 3 行。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 channels, %2 wavelengths (%3–%4 nm).</source>
        <translation>%1 个通道，%2 个波长（%3–%4 nm）。</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>The wavelengths in %1 do not increase.</source>
        <translation>%1 中的波长不是递增的。</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>%1 has no correction matrix (a variable correction, correctionMatrix or C with %2 columns).</source>
        <translation>%1 中没有校正矩阵（名为 correction、correctionMatrix 或 C 且有 %2 列的变量）。</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>%1 holds no matrix.</source>
        <translation>%1 中没有矩阵。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>%1: every row must have %2 values (one per channel), or %3 with the centre wavelength first.</source>
        <translation>%1：每行必须有 %2 个数值（每个通道一个），或 %3 个且第一个为中心波长。</translation>
    </message>
    <message>
        <location line="+68"/>
        <source>%1 is not a sensor profile.</source>
        <translation>%1 不是传感器配置文件。</translation>
    </message>
    <message>
        <location filename="../src/core/HsiCorrection.cpp" line="+22"/>
        <source>There is no image to correct.</source>
        <translation>没有可校正的图像。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The reference frames must have the same size as the image (%1 × %2): dark %3, white %4.</source>
        <translation>参考帧必须与图像尺寸相同（%1 × %2）：暗帧 %3，白参考帧 %4。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+1"/>
        <source>none</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Flat field: (image − dark) / (white − dark).</source>
        <translation>平场校正：(图像 − 暗帧) / (白参考 − 暗帧)。</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Flat field: image / white.</source>
        <translation>平场校正：图像 / 白参考。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark frame subtracted.</source>
        <translation>已扣除暗帧。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Exposure: scene %1 ms, white %2 ms.</source>
        <translation>曝光：场景 %1 ms，白参考 %2 ms。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1 pixels have no signal in the white frame and are left empty.</source>
        <translation>%1 个像素在白参考帧中无信号，保留为空。</translation>
    </message>
</context>
<context>
    <name>iat::isp</name>
    <message>
        <location filename="../src/core/Isp.cpp" line="+89"/>
        <source>px</source>
        <translation>像素</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>× noise σ</source>
        <translation>× 噪声 σ</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>×</source>
        <translation>×</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>stops</source>
        <translation>档</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Negative values</source>
        <translation>负值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clip at 0</source>
        <translation>截断为 0</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Keep</source>
        <translation>保留</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Correct</source>
        <translation>校正</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hot and dead pixels</source>
        <translation>亮点和暗点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hot pixels only</source>
        <translation>仅亮点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dead pixels only</source>
        <translation>仅暗点</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Direction</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rows and columns</source>
        <translation>行和列</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Rows</source>
        <translation>行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Columns</source>
        <translation>列</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Output depth</source>
        <translation>输出位深</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+186"/>
        <source>8-bit</source>
        <translation>8 位</translation>
    </message>
    <message>
        <location line="-186"/>
        <source>6-bit</source>
        <translation>6 位</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>4-bit</source>
        <translation>4 位</translation>
    </message>
    <message>
        <location line="+8"/>
        <location line="+1"/>
        <location line="+5"/>
        <source>Black level</source>
        <translation>黑电平</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>Subtracts the sensor&apos;s pedestal, so that no light reads as zero.</source>
        <translation>减去传感器的基底电平，使无光时读数为零。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fixed value</source>
        <translation>固定值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Subtracts a constant: the black level from the sensor&apos;s data sheet, or the mean of your dark frames.</source>
        <translation>减去一个常数：传感器数据手册给出的黑电平，或暗场帧的均值。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Automatic (low percentile)</source>
        <translation>自动（低百分位）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Uses a low percentile of the image as the black level. Only right when part of the image is truly dark.</source>
        <translation>以图像的某个低百分位值作为黑电平。仅当图像中确有全暗区域时才正确。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Percentile</source>
        <translation>百分位</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+1"/>
        <source>Defect pixels</source>
        <translation>坏点校正</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Replaces hot, dead and stuck pixels with a value from their neighbours.</source>
        <translation>用邻域像素的值替换亮点、暗点和固定值坏点。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Neighbour min / max</source>
        <translation>邻域最小 / 最大值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A pixel brighter than its brightest neighbour, or darker than its darkest, by more than the threshold is replaced by the median of the 8 neighbours. Leaves edges and thin lines alone. In textured areas the threshold rises with the local contrast.</source>
        <translation>若像素比最亮的邻点还亮、或比最暗的邻点还暗，且超出量大于阈值，则用 8 个邻点的中值替换。不影响边缘和细线。在纹理区域，阈值随局部对比度升高。</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+8"/>
        <location line="+14"/>
        <location line="+168"/>
        <source>Threshold</source>
        <translation>阈值</translation>
    </message>
    <message>
        <location line="-188"/>
        <source>Median threshold</source>
        <translation>中值阈值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A pixel that differs from the median of its 8 neighbours by more than the threshold is replaced by that median. Also catches pairs of defects, but can remove single-pixel detail such as stars. In textured areas the threshold rises with the local contrast.</source>
        <translation>若像素与其 8 个邻点中值之差超过阈值，则用该中值替换。也能处理成对的坏点，但可能去掉星点之类的单像素细节。在纹理区域，阈值随局部对比度升高。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Defect rows / columns</source>
        <translation>坏行 / 坏列校正</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Defect lines</source>
        <translation>坏行坏列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Repairs whole rows or columns that are dead, hot or offset from their neighbours.</source>
        <translation>修复整行或整列失效、过亮或相对邻行（列）存在偏移的情况。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Replace defective lines</source>
        <translation>替换坏行 / 坏列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A row or column that stands out from the lines on both sides of it (by more than the threshold, and by more than the pixel noise) is rebuilt by interpolating between the nearest good lines. Edges in the picture are left alone.</source>
        <translation>若某行或某列同时偏离其两侧的行（列），且偏离量超过阈值并大于像素噪声，则用最近的正常行（列）插值重建。图像中的边缘不受影响。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>× spread</source>
        <translation>× 离散度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove line offsets (destripe)</source>
        <translation>去除行 / 列偏移（去条纹）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Subtracts from every row or column its offset from a running median of the lines around it. Removes banding and row / column fixed-pattern noise.</source>
        <translation>从每一行或每一列中减去它相对周围行（列）滑动中值的偏移。可去除条带以及行 / 列固定模式噪声。</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+40"/>
        <source>Window</source>
        <translation>窗口</translation>
    </message>
    <message>
        <location line="-39"/>
        <source>lines</source>
        <translation>行</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+34"/>
        <location line="+25"/>
        <location line="+25"/>
        <source>Strength</source>
        <translation>强度</translation>
    </message>
    <message>
        <location line="-81"/>
        <source>Digital gain</source>
        <translation>数字增益</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+4"/>
        <source>Gain</source>
        <translation>增益</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>Multiplies every pixel by the same factor.</source>
        <translation>将每个像素乘以同一个系数。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Manual</source>
        <translation>手动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Multiplies by a fixed gain.</source>
        <translation>乘以固定增益。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Automatic (mean to target)</source>
        <translation>自动（均值到目标值）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Chooses the gain that brings the image mean to a target value, as auto exposure does.</source>
        <translation>选择使图像均值达到目标值的增益，类似自动曝光。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Target mean</source>
        <translation>目标均值</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+1"/>
        <source>Denoise</source>
        <translation>降噪</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reduces pixel noise. Strengths are relative to the noise level estimated from the image.</source>
        <translation>降低像素噪声。强度以从图像估计出的噪声水平为基准。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bilateral filter</source>
        <translation>双边滤波</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Averages only neighbours of similar value, so noise is smoothed while edges stay sharp.</source>
        <translation>只对数值相近的邻点取平均，因此在平滑噪声的同时保持边缘清晰。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Spatial radius (σ)</source>
        <translation>空间半径 (σ)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Range</source>
        <translation>值域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Guided filter</source>
        <translation>引导滤波</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Edge-preserving smoothing from local linear models. Fast at any radius, without the bilateral filter&apos;s gradient artefacts.</source>
        <translation>基于局部线性模型的保边平滑。任意半径下都很快，且没有双边滤波的梯度伪影。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+111"/>
        <source>Radius</source>
        <translation>半径</translation>
    </message>
    <message>
        <location line="-110"/>
        <location line="+27"/>
        <location line="+56"/>
        <location line="+29"/>
        <source>Edge threshold</source>
        <translation>边缘阈值</translation>
    </message>
    <message>
        <location line="-111"/>
        <source>Non-local means</source>
        <translation>非局部均值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Averages pixels whose surrounding patches look alike, anywhere in a search window. Keeps texture best; the slowest choice.</source>
        <translation>在搜索窗口内，对周围图像块相似的像素取平均。纹理保留得最好，但速度最慢。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Search radius</source>
        <translation>搜索半径</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Patch</source>
        <translation>图像块</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Median filter</source>
        <translation>中值滤波</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Each pixel becomes the median of its neighbourhood. Best for impulse (salt-and-pepper) noise; keeps edges but rounds corners.</source>
        <translation>每个像素取其邻域的中值。最适合脉冲（椒盐）噪声；保留边缘，但会使拐角变圆。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Gaussian blur</source>
        <translation>高斯模糊</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Weighted average of the neighbourhood. Fast, but blurs edges as much as noise.</source>
        <translation>邻域加权平均。速度快，但边缘会和噪声一样被模糊。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+115"/>
        <source>Radius (σ)</source>
        <translation>半径 (σ)</translation>
    </message>
    <message>
        <location line="-112"/>
        <source>Dynamic range compression</source>
        <translation>动态范围压缩</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dynamic range</source>
        <translation>动态范围</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reduces the contrast between bright and dark regions so that both fit the display. The result stays in DN, ready for tone mapping.</source>
        <translation>减小亮区与暗区之间的反差，使两者都能在显示范围内呈现。结果仍以 DN 为单位，可再进行色调映射。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Local (base / detail)</source>
        <translation>局部（基础层 / 细节层）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>In log space an edge-preserving filter separates a smooth base from the detail. Only the base is compressed, so textures keep their contrast.</source>
        <translation>在对数域中用保边滤波把平滑的基础层与细节层分开。只压缩基础层，因此纹理的对比度得以保留。</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+55"/>
        <source>Base contrast</source>
        <translation>基础层对比度</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+56"/>
        <source>Filter radius</source>
        <translation>滤波半径</translation>
    </message>
    <message>
        <location line="-54"/>
        <source>Global log curve</source>
        <translation>全局对数曲线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>out = log(1 + a·x) / log(1 + a): equal ratios of input become roughly equal steps of output.</source>
        <translation>out = log(1 + a·x) / log(1 + a)：输入的等比变化大致变为输出的等差变化。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Global gamma curve</source>
        <translation>全局伽马曲线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Raises the normalised image to the power 1/γ, which lifts the shadows.</source>
        <translation>对归一化图像取 1/γ 次幂，提亮暗部。</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+17"/>
        <source>Gamma</source>
        <translation>伽马</translation>
    </message>
    <message>
        <location line="-14"/>
        <location line="+1"/>
        <source>Tone mapping</source>
        <translation>色调映射</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Maps the full range of the data to the 256 grey levels of a display. The result is in display values, 0 to 255.</source>
        <translation>把数据的整个范围映射到显示器的 256 个灰度级。结果为显示值，0 到 255。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Percentile stretch</source>
        <translation>百分位拉伸</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Stretches the window between two percentiles of the histogram linearly to 0–255 (auto levels). Good for low-contrast data on a pedestal; everything outside the window clips.</source>
        <translation>把直方图两个百分位之间的窗口线性拉伸到 0–255（自动色阶）。适合带基底的低对比度数据；窗口之外的值全部被截断。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Clip darkest</source>
        <translation>截断最暗</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clip brightest</source>
        <translation>截断最亮</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Gamma curve</source>
        <translation>伽马曲线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Normalises to the robust min–max and raises to the power 1/γ. γ = 2.2 is roughly what a camera does for an sRGB image. Good for ordinary scenes of moderate range.</source>
        <translation>按稳健的最小–最大值归一化后取 1/γ 次幂。γ = 2.2 大致相当于相机生成 sRGB 图像时的处理。适合动态范围适中的普通场景。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Log curve</source>
        <translation>对数曲线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>out = log(1 + a·x) / log(1 + a). Every stop gets some codes; midtones look flat and shadow noise is amplified the most.</source>
        <translation>out = log(1 + a·x) / log(1 + a)。每一档曝光都分到一些灰度级；中间调显得平淡，暗部噪声被放大得最多。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Reinhard global</source>
        <translation>Reinhard 全局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scales the log-average of the image to a key value, compresses with L·(1 + L/L_white²)/(1 + L) and applies display gamma. A photographic look with a smooth highlight roll-off.</source>
        <translation>把图像的对数平均亮度缩放到基调值，用 L·(1 + L/L_white²)/(1 + L) 压缩，再施加显示伽马。具有摄影般的观感，高光过渡平滑。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Key</source>
        <translation>基调值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>White point</source>
        <translation>白点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>× max</source>
        <translation>× 最大值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Plateau equalisation</source>
        <translation>平台直方图均衡</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Histogram equalisation with every bin capped at a plateau, so large flat areas cannot take all the codes. Output codes follow the pixel population: the usual automatic gain of thermal cameras.</source>
        <translation>对每个直方图区间设置平台上限的直方图均衡，使大面积平坦区域无法占用全部灰度级。输出灰度级随像素分布而定：这是热像仪常用的自动增益方式。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Plateau limit</source>
        <translation>平台上限</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>× mean</source>
        <translation>× 均值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CLAHE</source>
        <translation>CLAHE</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Equalises each tile with a clipped histogram and blends the neighbouring tile curves. Shows detail everywhere at once; brightness is no longer comparable across the frame.</source>
        <translation>对每个分块用限幅直方图做均衡，并在相邻分块的曲线之间插值。可同时显示各处的细节；但画面不同位置的亮度不再可比。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tiles per side</source>
        <translation>每边分块数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clip limit</source>
        <translation>限幅</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Base / detail split</source>
        <translation>基础层 / 细节层分离</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>In log space a guided filter splits the image into a smooth base and a detail layer. Only the base is compressed; detail is added back with gain. The usual choice for real high dynamic range.</source>
        <translation>在对数域中用引导滤波把图像分为平滑的基础层和细节层。只压缩基础层，细节乘以增益后加回。这是真正高动态范围场景的常用选择。</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+30"/>
        <source>Detail gain</source>
        <translation>细节增益</translation>
    </message>
    <message>
        <location line="-27"/>
        <source>Exposure fusion</source>
        <translation>曝光融合</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Renders several virtual exposures from the one frame, weights each pixel by how close it is to mid-grey and blends them with Laplacian pyramids (Mertens). Natural and halo-resistant.</source>
        <translation>由同一帧生成多个虚拟曝光，按每个像素接近中灰的程度加权，再用拉普拉斯金字塔融合（Mertens）。效果自然，不易产生光晕。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Exposures</source>
        <translation>曝光数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exposure span</source>
        <translation>曝光跨度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Selectivity</source>
        <translation>选择性</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bit shift</source>
        <translation>移位</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Keeps the top 8 bits, so full scale maps linearly to 0–255. A reference point: nothing clips, but shadows collapse to a few codes.</source>
        <translation>保留最高 8 位，即把满量程线性映射到 0–255。可作为参照：不会截断，但暗部只剩下几个灰度级。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Input depth</source>
        <translation>输入位深</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Automatic</source>
        <translation>自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>10-bit</source>
        <translation>10 位</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>12-bit</source>
        <translation>12 位</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>14-bit</source>
        <translation>14 位</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>16-bit</source>
        <translation>16 位</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Detail enhancement</source>
        <translation>细节增强</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Detail</source>
        <translation>细节</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Amplifies fine local contrast (texture) without changing the large-scale brightness.</source>
        <translation>放大细微的局部对比度（纹理），而不改变大尺度的亮度。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Base / detail boost</source>
        <translation>基础层 / 细节层增强</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>A guided filter splits off the detail layer, which is multiplied by the gain. Detail below the noise threshold is left as it is, so noise is not amplified. Edges and slopes stronger than the edge threshold count as structure, not detail.</source>
        <translation>用引导滤波分离出细节层并乘以增益。低于噪声阈值的细节保持不变，因此不会放大噪声。强于边缘阈值的边缘和亮度渐变视为结构，而非细节。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Noise threshold</source>
        <translation>噪声阈值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Multi-scale</source>
        <translation>多尺度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Splits the image into fine, medium and coarse detail bands (differences of Gaussians), with a gain for each.</source>
        <translation>把图像分为细、中、粗三个细节频带（高斯差分），各自有一个增益。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fine (1 px)</source>
        <translation>细（1 像素）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Medium (4 px)</source>
        <translation>中（4 像素）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Coarse (16 px)</source>
        <translation>粗（16 像素）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Sharpening</source>
        <translation>锐化</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sharpen</source>
        <translation>锐化</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Steepens edges so the image looks crisper.</source>
        <translation>使边缘更陡，图像看起来更清晰。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Unsharp mask</source>
        <translation>反锐化掩模</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Adds back the difference between the image and a blurred copy. The threshold skips differences at noise level; limiting the overshoot prevents bright and dark halos along edges.</source>
        <translation>把图像与其模糊副本之差加回图像。阈值会跳过噪声水平的差异；限制过冲可避免边缘两侧出现亮暗光晕。</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+9"/>
        <source>Amount</source>
        <translation>数量</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Overshoot</source>
        <translation>过冲</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Allow</source>
        <translation>允许</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Limit to local min / max</source>
        <translation>限制在局部最小 / 最大值内</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Laplacian</source>
        <translation>拉普拉斯</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Subtracts the Laplacian (second derivative). The simplest sharpener: it acts on the finest scale only and amplifies noise.</source>
        <translation>减去拉普拉斯（二阶导数）。最简单的锐化方法：只作用于最细的尺度，并会放大噪声。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Output quantisation</source>
        <translation>输出量化</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Quantise</source>
        <translation>量化</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rounds the display values to whole grey levels. It needs tone mapping. Lower depths exaggerate banding, so you can see what dither does.</source>
        <translation>把显示值取整为整数灰度级。需要先开启色调映射。较低的位深会突出条带现象，便于观察抖动的作用。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Round</source>
        <translation>取整</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rounds to the nearest level. Smooth gradients can show bands.</source>
        <translation>取整到最近的灰度级。平滑的渐变可能出现条带。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Random dither (TPDF)</source>
        <translation>随机抖动 (TPDF)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Adds triangular random noise of ±1 level before rounding: bands turn into fine grain.</source>
        <translation>取整前加入 ±1 级的三角分布随机噪声：条带变成细腻的颗粒。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ordered dither (Bayer 8 × 8)</source>
        <translation>有序抖动（Bayer 8 × 8）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Adds a fixed 8 × 8 threshold pattern before rounding: a regular texture instead of bands.</source>
        <translation>取整前加入固定的 8 × 8 阈值图案：以规则的纹理代替条带。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Floyd–Steinberg</source>
        <translation>Floyd–Steinberg</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Passes each pixel&apos;s rounding error on to its neighbours (error diffusion).</source>
        <translation>把每个像素的取整误差传给相邻像素（误差扩散）。</translation>
    </message>
    <message>
        <location line="+615"/>
        <source>Subtracted %1 DN</source>
        <translation>已减去 %1 DN</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>%1 pixels corrected (%2 %)</source>
        <translation>已校正 %1 个像素（%2 %）</translation>
    </message>
    <message>
        <location line="+249"/>
        <source>%1 rows and %2 columns replaced</source>
        <translation>已替换 %1 行、%2 列</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Gain × %1</source>
        <translation>增益 × %1</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Noise σ about %1, then %2</source>
        <translation>噪声 σ 约 %1，处理后 %2</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Range %1 to %2 DN kept</source>
        <translation>保持范围 %1 至 %2 DN</translation>
    </message>
    <message>
        <location line="+196"/>
        <source>%1-bit input: divided by %2</source>
        <translation>%1 位输入：除以 %2</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Input window %1 to %2 DN</source>
        <translation>输入窗口 %1 至 %2 DN</translation>
    </message>
    <message>
        <location line="+165"/>
        <source>%1 grey levels</source>
        <translation>%1 个灰度级</translation>
    </message>
    <message>
        <location line="+90"/>
        <source>Skipped: turn on tone mapping first</source>
        <translation>已跳过：请先开启色调映射</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Ordinary scene</source>
        <translation>普通场景</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Defect pixel correction and a gamma curve: monotonic and predictable.</source>
        <translation>坏点校正加伽马曲线：单调且可预期。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bright and dark regions in one frame</source>
        <translation>同一画面中既有亮区又有暗区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Light denoising and exposure fusion, which compresses large-scale contrast and leaves texture alone.</source>
        <translation>轻度降噪加曝光融合：压缩大尺度反差，保留纹理。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Low contrast on a pedestal (thermal)</source>
        <translation>带基底的低对比度图像（热成像）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Destriping, plateau equalisation and a detail boost.</source>
        <translation>去条纹、平台直方图均衡和细节增强。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inspection: visibility first</source>
        <translation>检测：以可见性为先</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bilateral denoising and CLAHE, for the most local contrast everywhere.</source>
        <translation>双边降噪加 CLAHE，使各处的局部对比度最大。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sensor clean-up only</source>
        <translation>仅传感器校正</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Defect pixels and defect lines are repaired; the values stay in DN.</source>
        <translation>修复坏点和坏行 / 坏列；数值仍以 DN 为单位。</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../src/main.cpp" line="+35"/>
        <source>Dark-frame calibration and noise analysis for image stacks (MATLAB .mat, TIFF, PNG, JPEG)</source>
        <translation>对图像堆栈进行暗场校准和噪声分析（MATLAB .mat、TIFF、PNG、JPEG）</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Image to open in the first tab (the light frames, when dark frames are given too): a .mat file, or an image file (repeat the option for several images).</source>
        <translation>在第一个标签页中打开的图像（同时指定暗场帧时作为场景帧）：一个 .mat 文件，或一个图像文件（多张图像可重复使用该选项）。</translation>
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
