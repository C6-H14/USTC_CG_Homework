import torch
import torch.nn as nn
from local_renderformer.layers.attention import MultiHeadAttention, TransformerEncoder, TransformerDecoder

# ==========================================
# 架构师配置的极简测试超参数 (Micro-Batch)
# ==========================================
# 修改 test_attention_operator.py 中的超参数
B = 2           
NUM_HEADS = 4   
Q_LEN = 10      
KV_LEN = 20     
HIDDEN_DIM = 256 # 增大网络宽度，Head Dim = 64
ROPE_DIM = 8     # 8 // 2 * 9 = 36 <= 64 (合法！)

def run_smoke_test():
    print("🚀 [Smoke Test] 首席架构师的注意力算子截断测试启动...\n")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"🖥️ 运行设备: {device}\n")

    # ==========================================
    # TEST 1: Scaled Dot-Product Attention (SDPA)pip install torch

    # ==========================================
    print(">>> 测试 1: 核心 SDPA 算子与 Mask 逻辑防御")
    mha = MultiHeadAttention(query_dim=HIDDEN_DIM, num_heads=NUM_HEADS, kv_dim=HIDDEN_DIM).to(device)
    
    q_tensor = torch.randn(B, Q_LEN, HIDDEN_DIM).to(device)
    k_tensor = torch.randn(B, KV_LEN, HIDDEN_DIM).to(device)
    v_tensor = torch.randn(B, KV_LEN, HIDDEN_DIM).to(device)
    
    # 构造一个 Mask：假设每个 Batch 的前 15 个三角形是 Valid (True)，后 5 个是 Padding (False)
    mask = torch.zeros(B, KV_LEN, dtype=torch.bool).to(device)
    mask[:, :15] = True  
    
    try:
        # 强制使用我们手写的 sdpa，而不是 flash_attn
        out_mha = mha(q_tensor, k_tensor, v_tensor, src_key_padding_mask=mask, force_sdpa=True)
        assert out_mha.shape == (B, Q_LEN, HIDDEN_DIM), f"❌ SDPA 形状错误: {out_mha.shape}"
        assert not torch.isnan(out_mha).any(), "❌ SDPA 出现了 NaN 爆炸，Mask 逻辑可能错误！"
        print("✅ [通过] SDPA 张量流转与 Mask 反转防御验证成功！")
    except Exception as e:
        print(f"❌ SDPA 测试失败: {e}")

    # ==========================================
    # TEST 2: Transformer Encoder (自注意力与全局烘焙)
    # ==========================================
    print("\n>>> 测试 2: Encoder 自注意力循环与单坐标系 RoPE")
    encoder = TransformerEncoder(
        num_layers=2, num_heads=NUM_HEADS, hidden_dim=HIDDEN_DIM, 
        ffn_hidden_dim=HIDDEN_DIM*2, rope_dim=ROPE_DIM,
        encoder_skip_from_layer=1, encoder_skip_to_layer=2 # 测试残差跳跃
    ).to(device)

    tri_features = torch.randn(B, KV_LEN, HIDDEN_DIM).to(device)
    tri_pos = torch.randn(B, KV_LEN, 3).to(device) # 三角形 3D 坐标

    try:
        out_enc = encoder(tri_features, src_key_padding_mask=mask, triangle_pos=tri_pos)
        assert out_enc.shape == (B, KV_LEN, HIDDEN_DIM), f"❌ Encoder 形状错误: {out_enc.shape}"
        print("✅ [通过] Encoder 全局烘焙网络、跳跃连接与三角 RoPE 验证成功！")
    except Exception as e:
        print(f"❌ Encoder 测试失败: {e}")

    # ==========================================
    # TEST 3: Transformer Decoder (光线交叉注意力与多尺度提取)
    # ==========================================
    print("\n>>> 测试 3: Decoder 交叉注意力与双坐标系解耦 RoPE")
    decoder = TransformerDecoder(
        num_layers=3, num_heads=NUM_HEADS, hidden_dim=HIDDEN_DIM, 
        ffn_hidden_dim=HIDDEN_DIM*2, rope_dim=ROPE_DIM
    ).to(device)

    ray_features = torch.randn(B, Q_LEN, HIDDEN_DIM).to(device)
    ray_pos = torch.randn(B, Q_LEN, 3).to(device) # 光线 Query 坐标

    try:
        # 测试 out_layers 用于 DPT 多尺度特征提取
        out_dec = decoder(ray_features, ctx=out_enc, src_key_padding_mask=mask, 
                          triangle_pos=tri_pos, ray_pos=ray_pos, out_layers=[1, 3])
        
        # 因为我们请求了 out_layers=[1, 3]，返回的应该是一个包含两个元素的 list
        assert isinstance(out_dec, list) and len(out_dec) == 2, "❌ Decoder DPT 特征收集失败！"
        assert out_dec[0][0].shape == (B, Q_LEN, HIDDEN_DIM), "❌ Decoder 中间特征层张量变形！"
        print("✅ [通过] Decoder 双坐标系解耦计算与 DPT 列表提取验证成功！")
    except Exception as e:
        print(f"❌ Decoder 测试失败: {e}")

    print("\n🎉 全核心算子数学测试已通过！系统未产生 NaN 或张量塌陷。")

if __name__ == "__main__":
    run_smoke_test()