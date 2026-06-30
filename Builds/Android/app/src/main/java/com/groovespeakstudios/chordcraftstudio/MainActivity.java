package com.groovespeakstudios.chordcraftstudio;

import com.rmsl.juce.JuceActivity;
import android.os.Bundle;
import com.android.billingclient.api.*;
import com.google.android.gms.ads.*;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import androidx.annotation.NonNull;
import java.util.List;
import java.util.ArrayList;

public class MainActivity extends JuceActivity
{
    private BillingClient billingClient;
    private InterstitialAd mInterstitialAd;
    private boolean isProUnlocked = false;
    private final String PRO_PRODUCT_ID = "chordcraft_pro_unlock";

    // JNI Native Method Declaration
    public native void nativeSetProStatus(boolean isPro);

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        MobileAds.initialize(this, initializationStatus -> {
            loadInterstitialAd();
        });

        billingClient = BillingClient.newBuilder(this)
            .setListener(purchasesUpdatedListener)
            .enablePendingPurchases()
            .build();
            
        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    queryPurchases();
                }
            }
            @Override
            public void onBillingServiceDisconnected() {
                // Retry connection logic here if needed
            }
        });
    }

    public void showStartupAd() {
        runOnUiThread(() -> {
            if (!isProUnlocked && mInterstitialAd != null) {
                mInterstitialAd.show(this);
            }
        });
    }

    public void initiateProPurchase() {
        runOnUiThread(() -> {
            List<QueryProductDetailsParams.Product> productList = new ArrayList<>();
            productList.add(QueryProductDetailsParams.Product.newBuilder()
                .setProductId(PRO_PRODUCT_ID)
                .setProductType(BillingClient.ProductType.INAPP)
                .build());
                
            QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder().setProductList(productList).build();
            
            billingClient.queryProductDetailsAsync(params, (billingResult, productDetailsList) -> {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK && !productDetailsList.isEmpty()) {
                    BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder()
                        .setProductDetailsParamsList(List.of(BillingFlowParams.ProductDetailsParams.newBuilder()
                            .setProductDetails(productDetailsList.get(0)).build()))
                        .build();
                    billingClient.launchBillingFlow(this, billingFlowParams);
                }
            });
        });
    }

    private void loadInterstitialAd() {
        AdRequest adRequest = new AdRequest.Builder().build();
        InterstitialAd.load(this, "ca-app-pub-3940256099942544/1033173712", adRequest,
            new InterstitialAdLoadCallback() {
                @Override
                public void onAdLoaded(@NonNull InterstitialAd interstitialAd) { mInterstitialAd = interstitialAd; }
                @Override
                public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) { mInterstitialAd = null; }
            });
    }

    private void queryPurchases() {
        billingClient.queryPurchasesAsync(QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.INAPP).build(),
            (billingResult, purchases) -> {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    for (Purchase purchase : purchases) {
                        if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED && purchase.getProducts().contains(PRO_PRODUCT_ID)) {
                            isProUnlocked = true;
                            nativeSetProStatus(true);
                        }
                    }
                }
            });
    }

    private PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK && purchases != null) {
            for (Purchase purchase : purchases) {
                if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED && purchase.getProducts().contains(PRO_PRODUCT_ID)) {
                    isProUnlocked = true;
                    nativeSetProStatus(true);
                    // Note: Acknowledge purchase logic should go here in a production environment
                }
            }
        }
    };
}
